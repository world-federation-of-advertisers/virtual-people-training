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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common_cpp/protobuf_util/textproto_io.h"
#include "common_cpp/testing/common_matchers.h"
#include "common_cpp/testing/status_macros.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {
namespace {

using ::wfa::EqualsProto;
using ::wfa::IsOkAndHolds;
using ::wfa::ReadTextProtoFile;
using ::wfa::StatusIs;

TEST(CompileTest, ChildrenNotSet) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
      )pb",
      &config));
  EXPECT_THAT(CompileModel(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Children of the config is not set"));
}

TEST(CompileTest, StopNode) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        stop {}
      )pb",
      &config));
  CompiledNode expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branch_node {
          branches {
            node {
              name: "node1_stop"
              stop_node {}
            }
            condition { op: TRUE }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, BranchNoNode) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branches {}
      )pb",
      &config));
  EXPECT_THAT(
      CompileModel(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "No node in branches."));
}

TEST(CompileTest, BranchNoSelectBy) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branches {
          nodes {
            name: "node2"
            stop {}
          }
        }
      )pb",
      &config));
  EXPECT_THAT(CompileModel(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "select_by is not set for a branch"));
}

TEST(CompileTest, BranchNodeByChance) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branches {
          nodes {
            name: "node2"
            chance: 0.4
            stop {}
          }
          nodes {
            name: "node3"
            chance: 0.6
            stop {}
          }
        }
        random_seed: "seed1"
      )pb",
      &config));
  CompiledNode expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branch_node {
          branches {
            node {
              name: "node2"
              branch_node {
                branches {
                  node {
                    name: "node2_stop"
                    stop_node {}
                  }
                  condition { op: TRUE }
                }
              }
            }
            chance: 0.4
          }
          branches {
            node {
              name: "node3"
              branch_node {
                branches {
                  node {
                    name: "node3_stop"
                    stop_node {}
                  }
                  condition { op: TRUE }
                }
              }
            }
            chance: 0.6
          }
          random_seed: "seed1"
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, BranchNodeByChanceNoRandomSeed) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branches {
          nodes {
            name: "node2"
            chance: 0.4
            stop {}
          }
          nodes {
            name: "node3"
            chance: 0.6
            stop {}
          }
        }
      )pb",
      &config));
  EXPECT_THAT(CompileModel(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "random_seed must be set when branches are selected by "
                       "chances."));
}

TEST(CompileTest, BranchNodeByCondition) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branches {
          nodes {
            name: "node2"
            condition {
              from_file: "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/country_code_1_filter.textproto"
            }
            stop {}
          }
          nodes {
            name: "node3"
            condition {
              from_file: "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/true_filter.textproto"
            }
            stop {}
          }
        }
      )pb",
      &config));
  CompiledNode expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branch_node {
          branches {
            node {
              name: "node2"
              branch_node {
                branches {
                  node {
                    name: "node2_stop"
                    stop_node {}
                  }
                  condition { op: TRUE }
                }
              }
            }
            condition {
              op: EQUAL
              name: "person_country_code"
              value: "COUNTRY_CODE_1"
            }
          }
          branches {
            node {
              name: "node3"
              branch_node {
                branches {
                  node {
                    name: "node3_stop"
                    stop_node {}
                  }
                  condition { op: TRUE }
                }
              }
            }
            condition { op: TRUE }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, BranchNodeMixedSelectBy) {
  ModelNodeConfig config_1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branches {
          nodes {
            name: "node2"
            chance: 1
            stop {}
          }
          nodes {
            name: "node3"
            condition {
              from_file: "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/true_filter.textproto"
            }
            stop {}
          }
        }
        random_seed: "seed1"
      )pb",
      &config_1));
  EXPECT_THAT(CompileModel(config_1).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Not all branches has chance set"));

  ModelNodeConfig config_2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branches {
          nodes {
            name: "node2"
            condition {
              from_file: "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/true_filter.textproto"
            }
            stop {}
          }
          nodes {
            name: "node3"
            chance: 1
            stop {}
          }
        }
      )pb",
      &config_2));
  EXPECT_THAT(CompileModel(config_2).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Not all branches has condition set"));
}

TEST(CompileTest, AttributesUpdaters) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        stop {}
        updates {
          updates {
            update_matrix {
              from_file: "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/update_matrix.textproto"
            }
          }
        }
      )pb",
      &config));
  CompiledNode expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branch_node {
          branches {
            node {
              name: "node1_stop"
              stop_node {}
            }
            condition { op: TRUE }
          }
          updates {
            updates {
              update_matrix {
                columns { person_country_code: "COUNTRY_1" }
                columns { person_country_code: "COUNTRY_2" }
                rows { person_country_code: "UPDATED_COUNTRY_1" }
                rows { person_country_code: "UPDATED_COUNTRY_2" }
                probabilities: 0.8
                probabilities: 0.2
                probabilities: 0.2
                probabilities: 0.8
                pass_through_non_matches: false
                random_seed: "TestSeed"
              }
            }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, Multiplicity) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        stop {}
        multiplicity {
          from_file: "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/multiplicity.textproto"
        }
      )pb",
      &config));
  CompiledNode expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
        branch_node {
          branches {
            node {
              name: "node1_stop"
              stop_node {}
            }
            condition { op: TRUE }
          }
          multiplicity {
            expected_multiplicity: 1
            random_seed: "multiplicity_seed"
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, PopulationNode) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node.textproto",
          config)
          .ok());
  CompiledNode expected;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "compiled_node_for_population_node.textproto",
          expected)
          .ok());
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, PopulationNodeDiscretization) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node_discretization.textproto",
          config)
          .ok());
  CompiledNode expected;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "compiled_node_for_population_node_discretization.textproto",
          expected)
          .ok());
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, PopulationNodeRedistributeProbabilitiesForEmptyPools) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node_redistribute_probabilities_for_"
          "empty_pools.textproto",
          config)
          .ok());
  CompiledNode expected;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "compiled_node_for_population_node_redistribute_probabilities_for_"
          "empty_pools.textproto",
          expected)
          .ok());
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, PopulationNodeKappaLessThanOne) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node_kappa_less_than_one.textproto",
          config)
          .ok());
  CompiledNode expected;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "compiled_node_for_population_node_kappa_less_than_one.textproto",
          expected)
          .ok());
  EXPECT_THAT(CompileModel(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileTest, PopulationNodeNoCensus) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node_no_census.textproto",
          config)
          .ok());
  EXPECT_THAT(CompileModel(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Census records data is required to build population "
                       "pool"));
}

TEST(CompileTest, PopulationNodeNoAdf) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node_no_adf.textproto",
          config)
          .ok());
  EXPECT_THAT(CompileModel(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
}

TEST(CompileTest, PopulationNodeNoMultipool) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node_no_multipool.textproto",
          config)
          .ok());
  EXPECT_THAT(CompileModel(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
}

TEST(CompileTest, PopulationNodeAphaNotSumToOne) {
  ModelNodeConfig config;
  ASSERT_TRUE(
      ReadTextProtoFile(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "model_node_config_population_node_alpha_not_sum_to_one.textproto",
          config)
          .ok());
  EXPECT_THAT(CompileModel(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Input do not sum up to 1"));
}

}  // namespace
}  // namespace wfa_virtual_people
