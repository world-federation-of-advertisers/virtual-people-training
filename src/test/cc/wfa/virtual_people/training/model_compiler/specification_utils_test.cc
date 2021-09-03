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

#include "wfa/virtual_people/training/model_compiler/specification_utils.h"

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common_cpp/testing/common_matchers.h"
#include "common_cpp/testing/status_macros.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/common/field_filter.pb.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {
namespace {

using ::wfa::EqualsProto;
using ::wfa::IsOkAndHolds;
using ::wfa::StatusIs;

TEST(CompileFieldFilterProtoTest, FromVerbatim) {
  FieldFilterProtoSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        verbatim {
          op: EQUAL
          name: "person_country_code"
          value: "COUNTRY_CODE_1"
        }
      )pb",
      &config));
  FieldFilterProto expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: EQUAL name: "person_country_code" value: "COUNTRY_CODE_1"
      )pb",
      &expected));
  EXPECT_THAT(CompileFieldFilterProto(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileFieldFilterProtoTest, FromFile) {
  FieldFilterProtoSpecification config;
  config.set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "country_code_1_filter.textproto");
  FieldFilterProto expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: EQUAL name: "person_country_code" value: "COUNTRY_CODE_1"
      )pb",
      &expected));
  EXPECT_THAT(CompileFieldFilterProto(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileFieldFilterProtoTest, NotSet) {
  FieldFilterProtoSpecification config;
  EXPECT_THAT(CompileFieldFilterProto(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, ""));
}

TEST(CompileAttributesUpdatersTest, UpdateMatrixFromVerbatim) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          update_matrix {
            verbatim {
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
      )pb",
      &config));
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
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
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileAttributesUpdatersTest, UpdateMatrixFromFile) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_update_matrix()->set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "update_matrix.textproto");
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
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
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileFieldFilterProtoTest, UpdateMatrixNotSet) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_update_matrix();
  EXPECT_THAT(CompileAttributesUpdaters(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, ""));
}

TEST(CompileAttributesUpdatersTest, SparseUpdateMatrixFromVerbatim) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          sparse_update_matrix {
            verbatim {
              columns {
                column_attrs { person_country_code: "COUNTRY_1" }
                rows { person_country_code: "UPDATED_COUNTRY_1" }
                rows { person_country_code: "UPDATED_COUNTRY_2" }
                probabilities: 0.8
                probabilities: 0.2
              }
              columns {
                column_attrs { person_country_code: "COUNTRY_2" }
                rows { person_country_code: "UPDATED_COUNTRY_1" }
                rows { person_country_code: "UPDATED_COUNTRY_2" }
                probabilities: 0.2
                probabilities: 0.8
              }
              pass_through_non_matches: false
              random_seed: "TestSeed"
            }
          }
        }
      )pb",
      &config));
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          sparse_update_matrix {
            columns {
              column_attrs { person_country_code: "COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_2" }
              probabilities: 0.8
              probabilities: 0.2
            }
            columns {
              column_attrs { person_country_code: "COUNTRY_2" }
              rows { person_country_code: "UPDATED_COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_2" }
              probabilities: 0.2
              probabilities: 0.8
            }
            pass_through_non_matches: false
            random_seed: "TestSeed"
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileAttributesUpdatersTest, SparseUpdateMatrixFromFile) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_sparse_update_matrix()->set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "sparse_update_matrix.textproto");
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          sparse_update_matrix {
            columns {
              column_attrs { person_country_code: "COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_2" }
              probabilities: 0.8
              probabilities: 0.2
            }
            columns {
              column_attrs { person_country_code: "COUNTRY_2" }
              rows { person_country_code: "UPDATED_COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_2" }
              probabilities: 0.2
              probabilities: 0.8
            }
            pass_through_non_matches: false
            random_seed: "TestSeed"
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileFieldFilterProtoTest, SparseUpdateMatrixNotSet) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_sparse_update_matrix();
  EXPECT_THAT(CompileAttributesUpdaters(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, ""));
}

TEST(CompileAttributesUpdatersTest, ConditionalMergeFromVerbatim) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          conditional_merge {
            verbatim {
              nodes {
                condition {
                  op: EQUAL
                  name: "person_country_code"
                  value: "COUNTRY_1"
                }
                update { person_country_code: "UPDATED_COUNTRY_1" }
              }
              nodes {
                condition {
                  op: EQUAL
                  name: "person_country_code"
                  value: "COUNTRY_2"
                }
                update { person_country_code: "UPDATED_COUNTRY_2" }
              }
              pass_through_non_matches: false
            }
          }
        }
      )pb",
      &config));
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          conditional_merge {
            nodes {
              condition {
                op: EQUAL
                name: "person_country_code"
                value: "COUNTRY_1"
              }
              update { person_country_code: "UPDATED_COUNTRY_1" }
            }
            nodes {
              condition {
                op: EQUAL
                name: "person_country_code"
                value: "COUNTRY_2"
              }
              update { person_country_code: "UPDATED_COUNTRY_2" }
            }
            pass_through_non_matches: false
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileAttributesUpdatersTest, ConditionalMergeFromFile) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_conditional_merge()->set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "conditional_merge.textproto");
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          conditional_merge {
            nodes {
              condition {
                op: EQUAL
                name: "person_country_code"
                value: "COUNTRY_1"
              }
              update { person_country_code: "UPDATED_COUNTRY_1" }
            }
            nodes {
              condition {
                op: EQUAL
                name: "person_country_code"
                value: "COUNTRY_2"
              }
              update { person_country_code: "UPDATED_COUNTRY_2" }
            }
            pass_through_non_matches: false
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileFieldFilterProtoTest, ConditionalMergeNotSet) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_conditional_merge();
  EXPECT_THAT(CompileAttributesUpdaters(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, ""));
}

TEST(CompileAttributesUpdatersTest, ConditionalAssignmentFromVerbatim) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          conditional_assignment {
            verbatim {
              condition { op: HAS name: "acting_demo.gender" }
              assignments {
                source_field: "acting_demo.gender"
                target_field: "corrected_demo.gender"
              }
            }
          }
        }
      )pb",
      &config));
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          conditional_assignment {
            condition { op: HAS name: "acting_demo.gender" }
            assignments {
              source_field: "acting_demo.gender"
              target_field: "corrected_demo.gender"
            }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileAttributesUpdatersTest, ConditionalAssignmentFromFile) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_conditional_assignment()->set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "conditional_assignment.textproto");
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          conditional_assignment {
            condition { op: HAS name: "acting_demo.gender" }
            assignments {
              source_field: "acting_demo.gender"
              target_field: "corrected_demo.gender"
            }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileFieldFilterProtoTest, ConditionalAssignmentNotSet) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_conditional_assignment();
  EXPECT_THAT(CompileAttributesUpdaters(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, ""));
}

TEST(CompileAttributesUpdatersTest, UpdateTreeFromVerbatim) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          update_tree {
            root_node {
              verbatim {
                branch_node {
                  branches {
                    node { stop_node {} }
                    condition { op: TRUE }
                  }
                }
              }
            }
          }
        }
      )pb",
      &config));
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          update_tree {
            root {
              branch_node {
                branches {
                  node { stop_node {} }
                  condition { op: TRUE }
                }
              }
            }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileAttributesUpdatersTest, UpdateTreeCompiledNodeFromFile) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()
      ->mutable_update_tree()
      ->mutable_root_node()
      ->set_compiled_node_from_file(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "stop_node_tree.textproto");
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          update_tree {
            root {
              branch_node {
                branches {
                  node { stop_node {} }
                  condition { op: TRUE }
                }
              }
            }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileAttributesUpdatersTest, UpdateTreeFromModelNodeConfig) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          update_tree {
            root_node {
              model_node_config {
                name: "TestNode"
                stop {}
              }
            }
          }
        }
      )pb",
      &config));
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          update_tree {
            root {
              name: "TestNode"
              branch_node {
                branches {
                  node {
                    name: "TestNode_stop"
                    stop_node {}
                  }
                  condition { op: TRUE }
                }
              }
            }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileAttributesUpdatersTest, UpdateTreeModelNodeConfigFromFile) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()
      ->mutable_update_tree()
      ->mutable_root_node()
      ->set_model_node_config_from_file(
          "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
          "stop_node_tree_config.textproto");
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        updates {
          update_tree {
            root {
              name: "TestNode"
              branch_node {
                branches {
                  node {
                    name: "TestNode_stop"
                    stop_node {}
                  }
                  condition { op: TRUE }
                }
              }
            }
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileFieldFilterProtoTest, UpdateTreeNotSet) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_update_tree()->mutable_root_node();
  EXPECT_THAT(CompileAttributesUpdaters(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, ""));
}

TEST(CompileAttributesUpdatersTest, MultipleUpdates) {
  ModelNodeConfig::AttributesUpdatersSpecification config;
  config.add_updates()->mutable_update_matrix()->set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "update_matrix.textproto");
  config.add_updates()->mutable_sparse_update_matrix()->set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "sparse_update_matrix.textproto");
  BranchNode::AttributesUpdaters expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
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
        updates {
          sparse_update_matrix {
            columns {
              column_attrs { person_country_code: "COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_2" }
              probabilities: 0.8
              probabilities: 0.2
            }
            columns {
              column_attrs { person_country_code: "COUNTRY_2" }
              rows { person_country_code: "UPDATED_COUNTRY_1" }
              rows { person_country_code: "UPDATED_COUNTRY_2" }
              probabilities: 0.2
              probabilities: 0.8
            }
            pass_through_non_matches: false
            random_seed: "TestSeed"
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileAttributesUpdaters(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileMultiplicityTest, FromVerbatim) {
  MultiplicitySpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        verbatim { expected_multiplicity: 1 random_seed: "multiplicity_seed" }
      )pb",
      &config));
  Multiplicity expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        expected_multiplicity: 1 random_seed: "multiplicity_seed"
      )pb",
      &expected));
  EXPECT_THAT(CompileMultiplicity(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileMultiplicityTest, FromFile) {
  MultiplicitySpecification config;
  config.set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "multiplicity.textproto");
  Multiplicity expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        expected_multiplicity: 1 random_seed: "multiplicity_seed"
      )pb",
      &expected));
  EXPECT_THAT(CompileMultiplicity(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileMultiplicityTest, NotSet) {
  MultiplicitySpecification config;
  EXPECT_THAT(CompileMultiplicity(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, ""));
}

}  // namespace
}  // namespace wfa_virtual_people
