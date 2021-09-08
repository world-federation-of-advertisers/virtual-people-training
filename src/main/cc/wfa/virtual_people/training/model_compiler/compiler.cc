// Copyright 2021 The Cross-Media Measurement Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "wfa/virtual_people/training/model_compiler/compiler.h"

#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "common_cpp/macros/macros.h"
#include "glog/logging.h"
#include "wfa/virtual_people/common/field_filter.pb.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_compiler/field_filter_utils.h"
#include "wfa/virtual_people/training/model_compiler/specification_utils.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {

namespace {

// This stores some information that will be used when building child nodes.
struct CompilerContext {
  const CensusRecordsSpecification* census;
};

// Indicates whether the child node is selected by chance or condition.
class SelectBy {
 public:
  enum SelectBranchBy { kInvalid, kChance, kCondition };

  SelectBy() { by_ = kInvalid; }

  explicit SelectBy(const double chance) {
    by_ = kChance;
    chance_ = chance;
  }

  explicit SelectBy(const FieldFilterProtoSpecification& condition) {
    by_ = kCondition;
    condition_ = &condition;
  }

  SelectBranchBy GetBy() { return by_; }

  absl::StatusOr<double> GetChance() {
    if (by_ == kChance) {
      return chance_;
    }
    return absl::InternalError("Calling GetChance when chance is not set.");
  }

  absl::StatusOr<const FieldFilterProtoSpecification*> GetCondition() {
    if (by_ == kCondition) {
      return condition_;
    }
    return absl::InternalError(
        "Calling GetCondition when condition is not set.");
  }

 private:
  SelectBranchBy by_;
  double chance_;
  const FieldFilterProtoSpecification* condition_;
};

// This is forward-declared because of mutual recursion.
absl::StatusOr<SelectBy> CompileNode(const ModelNodeConfig& config,
                                     CompilerContext& context,
                                     CompiledNode& node);

// Create a BranchNode, with each branch compiled recursively from a
// ModelNodeConfigs.
absl::StatusOr<BranchNode> CompileBranchNode(const ModelNodeConfigs branches,
                                             absl::string_view random_seed,
                                             CompilerContext& context) {
  BranchNode branch_node;
  bool has_chance = false;
  bool has_condition = false;
  for (const ModelNodeConfig& config : branches.nodes()) {
    BranchNode::Branch* branch = branch_node.add_branches();
    ASSIGN_OR_RETURN(SelectBy select_by,
                     CompileNode(config, context, *branch->mutable_node()));
    switch (select_by.GetBy()) {
      case SelectBy::kChance: {
        if (has_condition) {
          return absl::InvalidArgumentError(absl::StrCat(
              "Both chance and condition are set for branch nodes: ",
              config.DebugString()));
        }
        has_chance = true;
        ASSIGN_OR_RETURN(double chance, select_by.GetChance());
        branch->set_chance(chance);
        break;
      }
      case SelectBy::kCondition: {
        if (has_chance) {
          return absl::InvalidArgumentError(absl::StrCat(
              "Both chance and condition are set for branch nodes: ",
              config.DebugString()));
        }
        has_condition = true;
        ASSIGN_OR_RETURN(const FieldFilterProtoSpecification* condition,
                         select_by.GetCondition());
        if (!condition) {
          return absl::InternalError(
              "NULL condition when selecting by conditon.");
        }
        ASSIGN_OR_RETURN(*branch->mutable_condition(),
                         CompileFieldFilterProto(*condition));
        break;
      }
      default:
        return absl::InvalidArgumentError(absl::StrCat(
            "select_by must be set for branch node: ", config.DebugString()));
    }
  }
  if (has_chance) {
    if (random_seed.empty()) {
      return absl::InvalidArgumentError(
          "random_seed must be set when branches are selected by chances.");
    }
    branch_node.set_random_seed(std::string(random_seed));
  }
  return branch_node;
}

// TODO(@tcsnfkx): Implement compiling population pool.
// Create an empty BranchNode currently.
absl::StatusOr<BranchNode> CompilePopulationPool() {
  LOG(ERROR) << "Compiling population pool is not implemented.";
  return BranchNode();
}

// Create a BranchNode with single branch, which is a stop node.
BranchNode CompileStop(absl::string_view name) {
  BranchNode stop;
  BranchNode::Branch* branch = stop.add_branches();
  *branch->mutable_condition() = CreateTrueFilter();
  CompiledNode* stop_node = branch->mutable_node();
  stop_node->set_name(absl::StrCat(name, "_stop"));
  stop_node->mutable_stop_node();
  return stop;
}

// Converts @config to @node. The child nodes are converted recursively.
// The return value indicates the chance/condition that the @node is selected
// from its parent node.
// Returns error status when the children of @config is not set.
absl::StatusOr<SelectBy> CompileNode(const ModelNodeConfig& config,
                                     CompilerContext& context,
                                     CompiledNode& node) {
  node.set_name(config.name());

  if (config.has_census()) {
    context.census = &config.census();
  }

  switch (config.children_case()) {
    case ModelNodeConfig::kBranches: {
      ASSIGN_OR_RETURN(
          *node.mutable_branch_node(),
          CompileBranchNode(config.branches(), config.random_seed(), context));
      break;
    }
    case ModelNodeConfig::kPopulationPoolConfig: {
      ASSIGN_OR_RETURN(*node.mutable_branch_node(), CompilePopulationPool());
      break;
    }
    case ModelNodeConfig::kStop: {
      *node.mutable_branch_node() = CompileStop(config.name());
      break;
    }
    default:
      return absl::InvalidArgumentError(absl::StrCat(
          "Children of the config is not set: ", config.DebugString()));
  }

  if (config.has_updates()) {
    ASSIGN_OR_RETURN(*node.mutable_branch_node()->mutable_updates(),
                     CompileAttributesUpdaters(config.updates()));
  }

  if (config.has_multiplicity()) {
    ASSIGN_OR_RETURN(*node.mutable_branch_node()->mutable_multiplicity(),
                     CompileMultiplicity(config.multiplicity()));
  }

  switch (config.select_by_case()) {
    case ModelNodeConfig::kChance:
      return SelectBy(config.chance());
    case ModelNodeConfig::kCondition:
      return SelectBy(config.condition());
    default:
      return SelectBy();
  }
}

}  // namespace

absl::StatusOr<CompiledNode> CompileModel(const ModelNodeConfig& config) {
  CompiledNode node;
  CompilerContext context;
  RETURN_IF_ERROR(CompileNode(config, context, node).status());
  return node;
}

}  // namespace wfa_virtual_people
