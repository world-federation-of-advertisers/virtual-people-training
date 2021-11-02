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
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "common_cpp/macros/macros.h"
#include "wfa/virtual_people/common/field_filter.pb.h"
#include "wfa/virtual_people/common/field_filter/field_filter.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_compiler/constants.h"
#include "wfa/virtual_people/training/model_compiler/field_filter_utils.h"
#include "wfa/virtual_people/training/model_compiler/specification_utils.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {

namespace {

// This stores some information that will be used when building child nodes.
struct CompilerContext {
  const CensusRecordsSpecification* census = nullptr;
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

  SelectBranchBy GetBy() const { return by_; }

  absl::StatusOr<double> GetChance() const {
    if (by_ == kChance) {
      return chance_;
    }
    return absl::InternalError("Calling GetChance when chance is not set.");
  }

  absl::StatusOr<const FieldFilterProtoSpecification*> GetCondition() const {
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
// ModelNodeConfigs. All branches must have chance set.
absl::StatusOr<BranchNode> CompileChanceBranchNode(
    const ModelNodeConfigs branches, absl::string_view random_seed,
    CompilerContext& context) {
  BranchNode branch_node;
  if (random_seed.empty()) {
    return absl::InvalidArgumentError(
        "random_seed must be set when branches are selected by chances.");
  }
  branch_node.set_random_seed(std::string(random_seed));
  for (const ModelNodeConfig& config : branches.nodes()) {
    BranchNode::Branch* branch = branch_node.add_branches();
    ASSIGN_OR_RETURN(SelectBy select_by,
                     CompileNode(config, context, *branch->mutable_node()));
    if (select_by.GetBy() != SelectBy::kChance) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Not all branches has chance set: ", branches.DebugString()));
    }
    ASSIGN_OR_RETURN(double chance, select_by.GetChance());
    branch->set_chance(chance);
  }
  return branch_node;
}

// Create a BranchNode, with each branch compiled recursively from a
// ModelNodeConfigs. All branches must have condition set.
absl::StatusOr<BranchNode> CompileConditionBranchNode(
    const ModelNodeConfigs branches, CompilerContext& context) {
  BranchNode branch_node;
  for (const ModelNodeConfig& config : branches.nodes()) {
    BranchNode::Branch* branch = branch_node.add_branches();
    ASSIGN_OR_RETURN(SelectBy select_by,
                     CompileNode(config, context, *branch->mutable_node()));
    if (select_by.GetBy() != SelectBy::kCondition) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Not all branches has condition set: ", branches.DebugString()));
    }
    ASSIGN_OR_RETURN(const FieldFilterProtoSpecification* condition,
                     select_by.GetCondition());
    if (!condition) {
      return absl::InternalError("NULL condition when selecting by condition.");
    }
    ASSIGN_OR_RETURN(*branch->mutable_condition(),
                     CompileFieldFilterProto(*condition));
  }
  return branch_node;
}

// Create a BranchNode, with each branch compiled recursively from a
// ModelNodeConfigs.
absl::StatusOr<BranchNode> CompileBranchNode(const ModelNodeConfigs branches,
                                             absl::string_view random_seed,
                                             CompilerContext& context) {
  if (branches.nodes_size() == 0) {
    return absl::InvalidArgumentError("No node in branches.");
  }
  switch (branches.nodes(0).select_by_case()) {
    case ModelNodeConfig::kChance:
      return CompileChanceBranchNode(branches, random_seed, context);
    case ModelNodeConfig::kCondition:
      return CompileConditionBranchNode(branches, context);
    default:
      return absl::InvalidArgumentError(absl::StrCat(
          "select_by is not set for a branch: ", branches.DebugString()));
  }
}

absl::Status ValidateAdf(const ActivityDensityFunction& adf) {
  if (adf.identifier_type_filters_size() != adf.identifier_type_names_size()) {
    return absl::InvalidArgumentError(
        "The count of identifier_type_filters and identifier_type_names must "
        "be the same in ADF.");
  }
  if (!adf.has_dirac_mixture()) {
    return absl::InvalidArgumentError("Dirac mixture must be set in ADF.");
  }
  if (adf.dirac_mixture().alphas_size() == 0) {
    return absl::InvalidArgumentError(
        "Alpha and Delta cannot be empty in Dirac mixture.");
  }
  if (adf.dirac_mixture().alphas_size() != adf.dirac_mixture().deltas_size()) {
    return absl::InvalidArgumentError(
        "The count of Alphas and Deltas mut be the same in Dirac mixture.");
  }
  for (const DiracDelta& delta : adf.dirac_mixture().deltas()) {
    if (delta.coordinates_size() != adf.identifier_type_filters_size()) {
      return absl::InvalidArgumentError(
          "The count of coordinates in Delta must be the same as the count of "
          "identifier_type_filters.");
    }
  }
  return absl::OkStatus();
}

using MultipoolRecordSet = absl::flat_hash_set<const MultipoolRecord*>;
using RegionRecordsMap = absl::flat_hash_map<std::string, MultipoolRecordSet>;
using GeoRecordsMap = absl::flat_hash_map<std::string, RegionRecordsMap>;

absl::StatusOr<GeoRecordsMap> GetCountryRegionMapFromMultipool(
    const Multipool& multipool) {
  GeoRecordsMap geo_multipool_map;
  for (const MultipoolRecord& record : multipool.records()) {
    ASSIGN_OR_RETURN(
        std::string country,
        GetValueOfEqualFilter(record.condition(), "person_country_code"));
    ASSIGN_OR_RETURN(
        std::string region,
        GetValueOfEqualFilter(record.condition(), "person_region_code"));
    geo_multipool_map[country][region].insert(&record);
  }
  return geo_multipool_map;
}

// Return the records from @records, which match @condition.
absl::StatusOr<std::vector<CensusRecord*>> GetMatchingRecords(
    CensusRecords& records, const FieldFilterProto& condition) {
  std::vector<CensusRecord*> matching;
  ASSIGN_OR_RETURN(std::unique_ptr<FieldFilter> filter,
                   FieldFilter::New(LabelerEvent().GetDescriptor(), condition));
  for (CensusRecord& record : *records.mutable_records()) {
    if (filter->IsMatch(record.attributes())) {
      matching.push_back(&record);
    }
  }
  return matching;
}

void Discretize(std::vector<CensusRecord*>& records,
                const uint64_t discretization) {
  for (CensusRecord* record : records) {
    record->set_total_population(
        static_cast<uint64_t>(record->total_population() / discretization) *
        discretization);
  }
}

bool IsDeviceMatch(const CensusRecord& record, const FieldFilter& filter) {
  // TODO(@tcsnfkx): Implement device match. Currently always return true.
  return true;
}

// Return the records from @records, which match the device filters in
// @condition.
absl::StatusOr<std::vector<CensusRecord*>> GetDeviceMatchingRecords(
    std::vector<CensusRecord*>& records, const FieldFilterProto& condition) {
  std::vector<CensusRecord*> matching;
  ASSIGN_OR_RETURN(std::unique_ptr<FieldFilter> filter,
                   FieldFilter::New(LabelerEvent().GetDescriptor(), condition));
  for (CensusRecord* record : records) {
    if (IsDeviceMatch(*record, *filter)) {
      matching.push_back(record);
    }
  }
  return matching;
}

uint64_t GetPopulationSum(const std::vector<CensusRecord*>& records) {
  uint64_t sum = 0;
  for (const CensusRecord* record : records) {
    sum += record->total_population();
  }
  return sum;
}

// Return error status if the @input is not normalized in the @allowed_error.
// Otherwise, normalize the @input.
absl::Status NormalizeIfInError(const double allowed_error,
                                std::vector<double>& input) {
  double total = std::accumulate(input.begin(), input.end(), 0.0);
  if (total < 1.0 - allowed_error || total > 1.0 + allowed_error) {
    return absl::InvalidArgumentError("Input do not sum up to 1.");
  }
  for (double& num : input) {
    num /= total;
  }
  return absl::OkStatus();
}

// Split @population_sum by the ratio of @alphas.
// @alphas should be normalized.
// The return is discretized.
std::vector<uint64_t> SplitPopulationByAlphas(const uint64_t population_sum,
                                              const std::vector<double>& alphas,
                                              const uint64_t discretization) {
  std::vector<double> boundaries(alphas.size() + 1, 0.0);
  for (int i = 0; i < alphas.size(); ++i) {
    boundaries[i + 1] = population_sum * alphas[i] + boundaries[i];
  }

  std::vector<uint64_t> discretized_boundaries(boundaries.size(), 0);
  for (int i = 0; i < boundaries.size(); ++i) {
    discretized_boundaries[i] = static_cast<uint64_t>(
        round(boundaries[i] / discretization) * discretization);
  }

  for (int i = 0; i < discretized_boundaries.size() - 1; ++i) {
    discretized_boundaries[i] =
        discretized_boundaries[i + 1] - discretized_boundaries[i];
  }
  discretized_boundaries.pop_back();
  return discretized_boundaries;
}

absl::StatusOr<std::vector<std::vector<PopulationNode::VirtualPersonPool>>>
SplitRecordsByDeltaPools(const std::vector<uint64_t>& delta_pool_sizes,
                         const std::vector<CensusRecord*>& records) {
  int next_record_index = 0;
  uint64_t current_record_start = 0;
  uint64_t current_record_remaining = 0;
  std::vector<std::vector<PopulationNode::VirtualPersonPool>> delta_pools;
  for (const uint64_t delta_pool_size : delta_pool_sizes) {
    std::vector<PopulationNode::VirtualPersonPool>& delta_pool =
        delta_pools.emplace_back();
    uint64_t need_to_fill = delta_pool_size;
    while (need_to_fill > 0) {
      if (current_record_remaining == 0) {
        // Current record is depleted. Get the next record.
        if (next_record_index == records.size()) {
          return absl::InternalError(
              "Total delta pool size is larger than total population.");
        }
        current_record_remaining =
            records[next_record_index]->total_population();
        current_record_start = records[next_record_index]->population_offset();
        ++next_record_index;
      }
      if (current_record_remaining == 0) {
        // Empty record.
        continue;
      }
      uint64_t fill_amount = std::min(need_to_fill, current_record_remaining);
      PopulationNode::VirtualPersonPool& virtual_person_pool =
          delta_pool.emplace_back();
      virtual_person_pool.set_population_offset(current_record_start);
      virtual_person_pool.set_total_population(fill_amount);
      need_to_fill -= fill_amount;
      current_record_remaining -= fill_amount;
      current_record_start += fill_amount;
    }
  }
  return delta_pools;
}

// Redistribute the probabilities from empty delta pools to other pools. The
// ratios of the non-empty pool sizes keep the same.
// @original_probabilities must be normalized.
// Sizes of @delta_pool_sizes and @original_probabilities must be the same.
std::vector<double> RedistributeProbabilitiesByDeltaPoolSizes(
    const std::vector<uint64_t>& delta_pool_sizes,
    const std::vector<double>& original_probabilities) {
  double kappa = std::accumulate(original_probabilities.begin(),
                                 original_probabilities.end() - 1, 0.0);
  std::vector<int> non_empty_indexes;
  for (int i = 0; i < delta_pool_sizes.size() - 1; ++i) {
    if (delta_pool_sizes[i] > 0) {
      non_empty_indexes.push_back(i);
    }
  }

  double non_empty_sum = 0.0;
  for (int i : non_empty_indexes) {
    non_empty_sum += original_probabilities[i];
  }

  std::vector<double> output(original_probabilities.size(), 0.0);
  output[original_probabilities.size() - 1] =
      original_probabilities[original_probabilities.size() - 1];
  for (int i : non_empty_indexes) {
    output[i] = original_probabilities[i] * kappa / non_empty_sum;
  }
  return output;
}

absl::Status CompileAdf(const ActivityDensityFunction& adf,
                        std::vector<CensusRecord*>& multipool_census,
                        CompiledNode& pool_node) {
  for (int i = 0; i < adf.identifier_type_filters_size(); ++i) {
    ASSIGN_OR_RETURN(std::vector<CensusRecord*> matching_census,
                     GetDeviceMatchingRecords(multipool_census,
                                              adf.identifier_type_filters(i)));
    if (matching_census.empty()) {
      continue;
    }
    Discretize(matching_census, kDiscretization);
    uint64_t population_sum = GetPopulationSum(matching_census);
    if (population_sum == 0) {
      return absl::InvalidArgumentError(
          "The total population of the matching census records is zero.");
    }

    std::vector<double> alphas(adf.dirac_mixture().alphas().begin(),
                               adf.dirac_mixture().alphas().end());
    RETURN_IF_ERROR(NormalizeIfInError(0.01, alphas));

    std::vector<uint64_t> delta_pool_sizes =
        SplitPopulationByAlphas(population_sum, alphas, kDiscretization);

    // Build delta pools.
    ASSIGN_OR_RETURN(
        std::vector<std::vector<PopulationNode::VirtualPersonPool>> delta_pools,
        SplitRecordsByDeltaPools(delta_pool_sizes, matching_census));

    // Build probabilities by delta pools.
    std::vector<double> original_probabilities(
        adf.dirac_mixture().alphas_size(), 0.0);
    for (int j = 0; j < adf.dirac_mixture().alphas_size(); ++j) {
      original_probabilities[j] = adf.dirac_mixture().alphas(j) *
                                  adf.dirac_mixture().deltas(j).coordinates(i);
    }
    RETURN_IF_ERROR(NormalizeIfInError(0.0001, original_probabilities));
    std::vector<double> probabilities_by_delta =
        RedistributeProbabilitiesByDeltaPoolSizes(delta_pool_sizes,
                                                  original_probabilities);

    BranchNode::Branch* identifier_branch =
        pool_node.mutable_branch_node()->add_branches();
    *identifier_branch->mutable_condition() = adf.identifier_type_filters(i);
    CompiledNode* identifier_node = identifier_branch->mutable_node();
    identifier_node->set_name(absl::StrCat(
        pool_node.name(), "_identifier_type_", adf.identifier_type_names(i)));

    identifier_node->mutable_branch_node()->set_random_seed(
        identifier_node->name());
    for (int j = 0; j < delta_pools.size(); ++j) {
      BranchNode::Branch* delta_branch =
          identifier_node->mutable_branch_node()->add_branches();
      delta_branch->set_chance(probabilities_by_delta[j]);
      CompiledNode* delta_node = delta_branch->mutable_node();
      delta_node->set_name(absl::StrCat(identifier_node->name(), "_delta_", j));
      delta_node->mutable_population_node()->mutable_pools()->Assign(
          delta_pools[j].begin(), delta_pools[j].end());
    }

    if (delta_pools.back().empty()) {
      // Build empty population pool for the case that kappa < 1.
      CompiledNode* last_delta_node =
          identifier_node->mutable_branch_node()
              ->mutable_branches(
                  identifier_node->branch_node().branches_size() - 1)
              ->mutable_node();
      if (last_delta_node->population_node().pools_size() != 0) {
        return absl::InternalError("Last delta should be empty.");
      }
      PopulationNode::VirtualPersonPool* empty_pool =
          last_delta_node->mutable_population_node()->add_pools();
      empty_pool->set_population_offset(0);
      empty_pool->set_total_population(0);
    }
  }
}

// Compiling population pool to a BranchNode.
absl::StatusOr<BranchNode> CompilePopulationPool(
    const PopulationPoolConfig& population_pool_config,
    const CompilerContext& context, absl::string_view name) {
  ASSIGN_OR_RETURN(
      ActivityDensityFunction adf,
      CompileActivityDensityFunction(population_pool_config.adf()));
  RETURN_IF_ERROR(ValidateAdf(adf));
  ASSIGN_OR_RETURN(Multipool multipool,
                   CompileMultipool(population_pool_config.multipool()));
  if (!context.census) {
    return absl::InvalidArgumentError(
        "Census records data is required to build population pool.");
  }
  ASSIGN_OR_RETURN(CensusRecords census, CompileCensusRecords(*context.census));

  ASSIGN_OR_RETURN(auto geo_multipool_map,
                   GetCountryRegionMapFromMultipool(multipool));

  BranchNode branch_node;
  for (const auto& [country, region_multipool_map] : geo_multipool_map) {
    BranchNode::Branch* country_branch = branch_node.add_branches();
    FieldFilterProto* country_condition = country_branch->mutable_condition();
    country_condition->set_op(FieldFilterProto::EQUAL);
    country_condition->set_name("person_country_code");
    country_condition->set_value(country);
    CompiledNode* country_node = country_branch->mutable_node();
    country_node->set_name(absl::StrCat(name, "_country_", country));

    for (const auto& [region, multipool_records] : region_multipool_map) {
      BranchNode::Branch* region_branch =
          country_node->mutable_branch_node()->add_branches();
      FieldFilterProto* region_condition = region_branch->mutable_condition();
      region_condition->set_op(FieldFilterProto::EQUAL);
      region_condition->set_name("person_region_code");
      region_condition->set_value(region);
      CompiledNode* region_node = region_branch->mutable_node();
      region_node->set_name(
          absl::StrCat(country_node->name(), "_region_", region));

      for (const MultipoolRecord* multipool_record : multipool_records) {
        BranchNode::Branch* pool_branch =
            region_node->mutable_branch_node()->add_branches();
        *pool_branch->mutable_condition() = multipool_record->condition();
        // Remove country and region conditions as already checked in parent
        // nodes.
        RemoveEqualFilter(*pool_branch->mutable_condition(),
                          "person_country_code");
        RemoveEqualFilter(*pool_branch->mutable_condition(),
                          "person_region_code");
        CompiledNode* pool_node = pool_branch->mutable_node();
        pool_node->set_name(absl::StrCat(region_node->name(), "_pool_",
                                         multipool_record->name()));

        ASSIGN_OR_RETURN(
            std::vector<CensusRecord*> multipool_census,
            GetMatchingRecords(census, multipool_record->condition()));

        RETURN_IF_ERROR(CompileAdf(adf, multipool_census, *pool_node));
      }
    }
  }

  return branch_node;
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
      ASSIGN_OR_RETURN(*node.mutable_branch_node(),
                       CompilePopulationPool(config.population_pool_config(),
                                             context, config.name()));
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
