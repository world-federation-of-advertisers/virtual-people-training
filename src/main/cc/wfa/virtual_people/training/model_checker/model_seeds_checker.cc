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

#include "wfa/virtual_people/training/model_checker/model_seeds_checker.h"

#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "common_cpp/macros/macros.h"
#include "wfa/virtual_people/common/model.pb.h"

namespace wfa_virtual_people {

// Get the map from a node to its parent node, both are represented by their
// indexes in @nodes.
absl::StatusOr<absl::flat_hash_map<int, int>> GetParentIndexMap(
    const std::vector<CompiledNode>& nodes) {
  // A map from the CompiledNode.index to the index in @nodes.
  absl::flat_hash_map<uint32_t, int> node_index_to_vector_index;
  for (int i = 0; i < nodes.size(); ++i) {
    if (nodes[i].has_index()) {
      node_index_to_vector_index[nodes[i].index()] = i;
    }
  }

  absl::flat_hash_map<int, int> parent_vector_index;
  for (int i = 0; i < nodes.size(); ++i) {
    if (!nodes[i].has_branch_node()) {
      continue;
    }
    for (const BranchNode::Branch& branch : nodes[i].branch_node().branches()) {
      if (!branch.has_node_index()) {
        return absl::FailedPreconditionError(
            absl::StrCat("This node contains branch not referenced by index: ",
                         nodes[i].DebugString()));
      }
      // Get the index in @nodes of the child node.
      auto it = node_index_to_vector_index.find(branch.node_index());
      if (it == node_index_to_vector_index.end()) {
        return absl::FailedPreconditionError(
            absl::StrCat("This node refers to non-existing child node: ",
                         nodes[i].DebugString()));
      }
      parent_vector_index[it->second] = i;
    }
  }
  return parent_vector_index;
}

// Get the random seeds in @branch_node.
absl::flat_hash_set<std::string> GetRandomSeedsForBranchNode(
    const BranchNode& branch_node) {
  absl::flat_hash_set<std::string> random_seeds;
  if (branch_node.has_random_seed()) {
    random_seeds.insert(branch_node.random_seed());
  }
  if (branch_node.has_updates()) {
    for (const BranchNode::AttributesUpdater& updater :
         branch_node.updates().updates()) {
      if (updater.has_update_matrix()) {
        const UpdateMatrix& update_matrix = updater.update_matrix();
        if (update_matrix.has_random_seed()) {
          random_seeds.insert(update_matrix.random_seed());
        }
      } else if (updater.has_sparse_update_matrix()) {
        const SparseUpdateMatrix& sparse_update_matrix =
            updater.sparse_update_matrix();
        if (sparse_update_matrix.has_random_seed()) {
          random_seeds.insert(sparse_update_matrix.random_seed());
        }
      }
    }
  } else if (branch_node.has_multiplicity()) {
    const Multiplicity& multiplicity = branch_node.multiplicity();
    if (multiplicity.has_random_seed()) {
      random_seeds.insert(multiplicity.random_seed());
    }
  }
  return random_seeds;
}

// Get the random seeds in @population_node.
absl::flat_hash_set<std::string> GetRandomSeedsForPopulationNode(
    const PopulationNode& population_node) {
  absl::flat_hash_set<std::string> random_seeds;
  if (population_node.has_random_seed()) {
    random_seeds.insert(population_node.random_seed());
  }
  return random_seeds;
}

// Get all the random seeds from the node.
absl::flat_hash_set<std::string> GetRandomSeeds(const CompiledNode& node) {
  if (node.has_branch_node()) {
    return GetRandomSeedsForBranchNode(node.branch_node());
  } else if (node.has_population_node()) {
    return GetRandomSeedsForPopulationNode(node.population_node());
  }
  return absl::flat_hash_set<std::string>();
}

// Get the random seeds for all nodes, and store the seeds in the same order as
// the nodes.
std::vector<absl::flat_hash_set<std::string>> GetRandomSeedsForAllNodes(
    const std::vector<CompiledNode>& nodes) {
  std::vector<absl::flat_hash_set<std::string>> all_seeds;
  for (const CompiledNode& node : nodes) {
    all_seeds.emplace_back(GetRandomSeeds(node));
  }
  return all_seeds;
}

absl::Status CheckNodeSeeds(const std::vector<CompiledNode>& nodes) {
  ASSIGN_OR_RETURN(auto parent_vector_index, GetParentIndexMap(nodes));
  std::vector<absl::flat_hash_set<std::string>> all_seeds =
      GetRandomSeedsForAllNodes(nodes);
  // To store the indexes of the nodes that has any duplicated random seed.
  std::vector<int> violation_indexes;
  for (int i = 0; i < nodes.size(); ++i) {
    const absl::flat_hash_set<std::string>& random_seeds = all_seeds.at(i);
    // Go through all the ancestors to the root, and check if there is any
    // duplicated random seed in the ancestors.
    int current_index = i;
    while (true) {
      // Get the parent of the current node to be the current ancestor node.
      auto it = parent_vector_index.find(current_index);
      if (it == parent_vector_index.end()) {
        break;
      }
      current_index = it->second;
      // Get the random seeds of the current ancestor node.
      const absl::flat_hash_set<std::string>& ancestor_random_seeds =
          all_seeds.at(current_index);
      // Check if the random seeds of the node i have any duplicates among the
      // random seeds of the current ancestor node.
      bool has_duplicate = false;
      for (const std::string& random_seed : random_seeds) {
        if (ancestor_random_seeds.contains(random_seed)) {
          has_duplicate = true;
          violation_indexes.push_back(i);
          break;
        }
      }
      if (has_duplicate) {
        break;
      }
    }
  }

  if (violation_indexes.empty()) {
    return absl::OkStatus();
  }

  std::string error_message =
      "Each of the following nodes has duplicated random seeds in their "
      "ancestors:\n";
  for (int index : violation_indexes) {
    absl::StrAppend(&error_message, "\n", nodes[index].DebugString(), "\n");
  }
  return absl::InvalidArgumentError(error_message);
}

}  // namespace wfa_virtual_people
