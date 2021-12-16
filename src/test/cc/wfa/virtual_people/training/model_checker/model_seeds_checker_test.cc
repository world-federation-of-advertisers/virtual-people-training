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

#include "absl/status/status.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/common/model.pb.h"

namespace wfa_virtual_people {
namespace {

using ::wfa::IsOk;
using ::wfa::StatusIs;

TEST(CheckNodeSeedsTest, NoDuplicatedSeed) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        index: 1
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
        index: 2
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_2"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node3"
        index: 3
        branch_node {
          branches {
            node_index: 1
            chance: 0.5
          }
          branches {
            node_index: 2
            chance: 0.5
          }
          random_seed: "seed_3"
        }
      )pb",
      &nodes.emplace_back()));
  EXPECT_THAT(CheckNodeSeeds(nodes), IsOk());
}

TEST(CheckNodeSeedsTest, SingleDuplicatedSeed) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        index: 1
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
        index: 2
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_2"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node3"
        index: 3
        branch_node {
          branches {
            node_index: 1
            chance: 0.5
          }
          branches {
            node_index: 2
            chance: 0.5
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  std::string expected_error_message =
      "Each of the following nodes has duplicated random seeds in their "
      "ancestors:\n"
      "\n"
      "name: \"node1\"\n"
      "index: 1\n"
      "population_node {\n"
      "  pools {\n"
      "    population_offset: 1000\n"
      "    total_population: 100\n"
      "  }\n"
      "  random_seed: \"seed_1\"\n"
      "}\n"
      "\n";
  EXPECT_THAT(CheckNodeSeeds(nodes),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       expected_error_message));
}

TEST(CheckNodeSeedsTest, MultipleDuplicatedSeed) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        index: 1
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
        index: 2
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node3"
        index: 3
        branch_node {
          branches {
            node_index: 1
            chance: 0.5
          }
          branches {
            node_index: 2
            chance: 0.5
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  std::string expected_error_message =
      "Each of the following nodes has duplicated random seeds in their "
      "ancestors:\n"
      "\n"
      "name: \"node1\"\n"
      "index: 1\n"
      "population_node {\n"
      "  pools {\n"
      "    population_offset: 1000\n"
      "    total_population: 100\n"
      "  }\n"
      "  random_seed: \"seed_1\"\n"
      "}\n"
      "\n"
      "\n"
      "name: \"node2\"\n"
      "index: 2\n"
      "population_node {\n"
      "  pools {\n"
      "    population_offset: 1000\n"
      "    total_population: 100\n"
      "  }\n"
      "  random_seed: \"seed_1\"\n"
      "}\n"
      "\n";
  EXPECT_THAT(CheckNodeSeeds(nodes),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       expected_error_message));
}

TEST(CheckNodeSeedsTest, DuplicatedSeedInNotImmediateAncestor) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        index: 1
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
        index: 2
        branch_node {
          branches {
            node_index: 1
            chance: 1
          }
          random_seed: "seed_2"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node3"
        index: 3
        branch_node {
          branches {
            node_index: 2
            chance: 1
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  std::string expected_error_message =
      "Each of the following nodes has duplicated random seeds in their "
      "ancestors:\n"
      "\n"
      "name: \"node1\"\n"
      "index: 1\n"
      "population_node {\n"
      "  pools {\n"
      "    population_offset: 1000\n"
      "    total_population: 100\n"
      "  }\n"
      "  random_seed: \"seed_1\"\n"
      "}\n"
      "\n";
  EXPECT_THAT(CheckNodeSeeds(nodes),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       expected_error_message));
}

TEST(CheckNodeSeedsTest, DuplicatedSeedInUpdateMatrix) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        index: 1
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
        index: 2
        branch_node {
          branches {
            node_index: 1
            chance: 1
          }
          random_seed: "seed_2"
          updates { updates { update_matrix {
            random_seed: "seed_1"
          }}}
        }
      )pb",
      &nodes.emplace_back()));
  std::string expected_error_message =
      "Each of the following nodes has duplicated random seeds in their "
      "ancestors:\n"
      "\n"
      "name: \"node1\"\n"
      "index: 1\n"
      "population_node {\n"
      "  pools {\n"
      "    population_offset: 1000\n"
      "    total_population: 100\n"
      "  }\n"
      "  random_seed: \"seed_1\"\n"
      "}\n"
      "\n";
  EXPECT_THAT(CheckNodeSeeds(nodes),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       expected_error_message));
}

TEST(CheckNodeSeedsTest, DuplicatedSeedInSparseUpdateMatrix) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        index: 1
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
        index: 2
        branch_node {
          branches {
            node_index: 1
            chance: 1
          }
          random_seed: "seed_2"
          updates { updates { sparse_update_matrix {
            random_seed: "seed_1"
          }}}
        }
      )pb",
      &nodes.emplace_back()));
  std::string expected_error_message =
      "Each of the following nodes has duplicated random seeds in their "
      "ancestors:\n"
      "\n"
      "name: \"node1\"\n"
      "index: 1\n"
      "population_node {\n"
      "  pools {\n"
      "    population_offset: 1000\n"
      "    total_population: 100\n"
      "  }\n"
      "  random_seed: \"seed_1\"\n"
      "}\n"
      "\n";
  EXPECT_THAT(CheckNodeSeeds(nodes),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       expected_error_message));
}

TEST(CheckNodeSeedsTest, DuplicatedSeedInMultiplicity) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        index: 1
        population_node {
          pools {
            population_offset: 1000
            total_population: 100
          }
          random_seed: "seed_1"
        }
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
        index: 2
        branch_node {
          branches {
            node_index: 1
            chance: 1
          }
          random_seed: "seed_2"
          multiplicity {
            random_seed: "seed_1"
          }
        }
      )pb",
      &nodes.emplace_back()));
  std::string expected_error_message =
      "Each of the following nodes has duplicated random seeds in their "
      "ancestors:\n"
      "\n"
      "name: \"node1\"\n"
      "index: 1\n"
      "population_node {\n"
      "  pools {\n"
      "    population_offset: 1000\n"
      "    total_population: 100\n"
      "  }\n"
      "  random_seed: \"seed_1\"\n"
      "}\n"
      "\n";
  EXPECT_THAT(CheckNodeSeeds(nodes),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       expected_error_message));
}

}  // namespace
}  // namespace wfa_virtual_people
