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
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
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
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
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
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
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
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
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
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
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
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "None of verbatim, compiled_node_from_file, "
                       "model_node_config, or model_node_config_from_file is "
                       "set"));
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
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
}

TEST(CompileActivityDensityFunctionTest, FromVerbatim) {
  ActivityDensityFunctionSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        verbatim {
          name: "TestAdf"
          identifier_type_filters { op: TRUE }
          identifier_type_names: "TestIdType"
          dirac_mixture {
            alphas: 0
            deltas { coordinates: 0 }
          }
        }
      )pb",
      &config));
  ActivityDensityFunction expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "TestAdf"
        identifier_type_filters { op: TRUE }
        identifier_type_names: "TestIdType"
        dirac_mixture {
          alphas: 0
          deltas { coordinates: 0 }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileActivityDensityFunction(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileActivityDensityFunctionTest, FromFile) {
  ActivityDensityFunctionSpecification config;
  config.set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "activity_density_function.textproto");
  ActivityDensityFunction expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "TestAdf"
        identifier_type_filters { op: TRUE }
        identifier_type_names: "TestIdType"
        dirac_mixture {
          alphas: 0
          deltas { coordinates: 0 }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileActivityDensityFunction(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileActivityDensityFunctionTest, NotSet) {
  ActivityDensityFunctionSpecification config;
  EXPECT_THAT(CompileActivityDensityFunction(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
}

TEST(CompileMultipoolTest, FromVerbatim) {
  MultipoolSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        verbatim {
          records {
            name: "COUNTRY_CODE_1_filter"
            condition {
              op: EQUAL
              name: "person_country_code"
              value: "COUNTRY_CODE_1"
            }
          }
        }
      )pb",
      &config));
  Multipool expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        records {
          name: "COUNTRY_CODE_1_filter"
          condition {
            op: EQUAL
            name: "person_country_code"
            value: "COUNTRY_CODE_1"
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileMultipool(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileMultipoolTest, FromFile) {
  MultipoolSpecification config;
  config.set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "multipool.textproto");
  Multipool expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        records {
          name: "COUNTRY_CODE_1_filter"
          condition {
            op: EQUAL
            name: "person_country_code"
            value: "COUNTRY_CODE_1"
          }
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileMultipool(config), IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileMultipoolTest, NotSet) {
  MultipoolSpecification config;
  EXPECT_THAT(CompileMultipool(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
}

TEST(CompileCensusRecordsTest, FromVerbatim) {
  CensusRecordsSpecification config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        verbatim {
          records {
            attributes { person_country_code: "COUNTRY_CODE_1" }
            population_offset: 0
            total_population: 100
          }
        }
      )pb",
      &config));
  CensusRecords expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        records {
          attributes { person_country_code: "COUNTRY_CODE_1" }
          population_offset: 0
          total_population: 100
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileCensusRecords(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileCensusRecordsTest, FromFile) {
  CensusRecordsSpecification config;
  config.set_from_file(
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "census_records.textproto");
  CensusRecords expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        records {
          attributes { person_country_code: "COUNTRY_CODE_1" }
          population_offset: 0
          total_population: 100
        }
      )pb",
      &expected));
  EXPECT_THAT(CompileCensusRecords(config),
              IsOkAndHolds(EqualsProto(expected)));
}

TEST(CompileCensusRecordsTest, NotSet) {
  CensusRecordsSpecification config;
  EXPECT_THAT(CompileCensusRecords(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Neither verbatim nor from_file is set"));
}

}  // namespace
}  // namespace wfa_virtual_people
