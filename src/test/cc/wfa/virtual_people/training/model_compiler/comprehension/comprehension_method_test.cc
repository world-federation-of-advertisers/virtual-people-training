// Copyright 2022 The Cross-Media Measurement Authors
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

#include "wfa/virtual_people/training/model_compiler/comprehension/comprehension_method.h"

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common_cpp/testing/common_matchers.h"
#include "common_cpp/testing/status_macros.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/training/comprehend.pb.h"
#include "wfa/virtual_people/training/model_compiler/comprehension/contextual_boolean_expression.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {
namespace {

using ::wfa::EqualsProto;

using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;
using ::wfa::StatusIs;

TEST(ComprehensionMethodTest, ForEach) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation: {country}"
            comprehend {
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "CC" items: "AA" items: "BB" } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes { name: "Nation: CC" }
          nodes { name: "Nation: AA" }
          nodes { name: "Nation: BB" }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, ForEach_Multiple) {
  // Combine several for_each list.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation: {country}"
            comprehend {
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "CC" items: "AA" items: "BB" } }
                }
              }
            }
          }
          nodes {
            name: "City: {city}"
            comprehend {
              methods {
                for_each {
                  entity: "city"
                  values { verbatim { items: "XX" items: "YY" items: "ZZ" } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes { name: "Nation: CC" }
          nodes { name: "Nation: AA" }
          nodes { name: "Nation: BB" }
          nodes { name: "City: XX" }
          nodes { name: "City: YY" }
          nodes { name: "City: ZZ" }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, SetValues) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation {country} with size {people} and area {area}"
            comprehend {
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "AA" items: "BB" } }
                }
              }
              methods {
                set_values {
                  keys_to_assign_values: "area"
                  keys_to_assign_values: "people"
                  key_to_retrieve_values: "country"
                  map_spec {
                    verbatim {
                      items { key: "AA" values: "900" values: "300 million" }
                      items { key: "BB" values: "1000" values: "30 million" }
                    }
                  }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes { name: "Nation AA with size 300 million and area 900" }
          nodes { name: "Nation BB with size 30 million and area 1000" }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, SetValues_RetrieveKeyNotDefined) {
  // If key_to_retrieve_values is not defined, values to assign are fixed.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation {country} with size {people} and area {area}"
            comprehend {
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "AA" items: "BB" } }
                }
              }
              methods {
                set_values {
                  keys_to_assign_values: "area"
                  keys_to_assign_values: "people"
                  map_spec {
                    verbatim { items { values: "900" values: "300 million" } }
                  }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes { name: "Nation AA with size 300 million and area 900" }
          nodes { name: "Nation BB with size 300 million and area 900" }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, Filter) {
  // Note that contextual expression cannot compare to constant directly. Must
  // add to context first.
  // For example, to test country == 'AA', add
  // item { key: 'AA' value: 'AA' }
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation {country}"
            comprehend {
              context {
                items { key: "AA" value: "AA" }
                items { key: "BB" value: "BB" }
              }
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "AA" items: "BB" } }
                }
              }
            }
            branches {
              nodes {
                name: "{country} type1"
                comprehend {
                  methods {
                    filter {
                      expression {
                        not_expression {
                          expression {
                            equality { left_key: "country" right_key: "AA" }
                          }
                        }
                      }
                    }
                  }
                }
              }
              nodes {
                name: "{country} type2"
                comprehend {
                  methods {
                    filter {
                      expression {
                        not_expression {
                          expression {
                            equality { left_key: "country" right_key: "BB" }
                          }
                        }
                      }
                    }
                  }
                }
              }
              nodes { name: "{country} type3" }
            }
          }
        }
      )pb",
      &config));
  // type1 filters out AA, type2 filters out BB, type3 has no filter
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation AA"
            branches {
              nodes { name: "AA type2" }
              nodes { name: "AA type3" }
            }
          }
          nodes {
            name: "Nation BB"
            branches {
              nodes { name: "BB type1" }
              nodes { name: "BB type3" }
            }
          }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, ApplyIf) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "{name_prefix} Nation {country} {name_suffix}"
            comprehend {
              context {
                items { key: "AA" value: "AA" }
                items { key: "BB" value: "BB" }
              }
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "AA" items: "BB" items: "CC" } }
                }
              }
              methods {
                apply_if {
                  condition { equality { left_key: "country" right_key: "AA" } }
                  if_method {
                    set_values {
                      keys_to_assign_values: "name_prefix"
                      keys_to_assign_values: "name_suffix"
                      map_spec {
                        verbatim { items { values: "111", values: "999" } }
                      }
                    }
                  }
                  else_method {
                    set_values {
                      keys_to_assign_values: "name_prefix"
                      keys_to_assign_values: "name_suffix"
                      map_spec {
                        verbatim { items { values: "222", values: "666" } }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      )pb",
      &config));
  // AA uses if branch, others uses else branch
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes { name: "111 Nation AA 999" }
          nodes { name: "222 Nation BB 666" }
          nodes { name: "222 Nation CC 666" }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, ApplyIf_NoElseMethod) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "{name_prefix} Nation {country} {name_suffix}"
            comprehend {
              context {
                items { key: "AA" value: "AA" }
                items { key: "BB" value: "BB" }
              }
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "AA" items: "BB" items: "CC" } }
                }
              }
              methods {
                apply_if {
                  condition { equality { left_key: "country" right_key: "AA" } }
                  if_method {
                    set_values {
                      keys_to_assign_values: "name_prefix"
                      keys_to_assign_values: "name_suffix"
                      map_spec {
                        verbatim { items { values: "111", values: "999" } }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      )pb",
      &config));
  // AA uses if branch. No else branch.
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes { name: "111 Nation AA 999" }
          nodes { name: "{name_prefix} Nation BB {name_suffix}" }
          nodes { name: "{name_prefix} Nation CC {name_suffix}" }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, FormatCondition) {
  // Strings in other fields, such as condition, should be formatted too.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation {country} {no-match}"
            condition {
              verbatim { op: EQUAL name: "some_field" value: "{target_value}" }
            }
            comprehend {
              methods {
                for_each {
                  entity: "country"
                  values { verbatim { items: "AA" items: "BB" } }
                }
              }
              methods {
                set_values {
                  keys_to_assign_values: "target_value"
                  key_to_retrieve_values: "country"
                  map_spec {
                    verbatim {
                      items { key: "AA" values: "900" }
                      items { key: "BB" values: "1000" }
                      items { key: "CC" values: "2000" }
                    }
                  }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "World"
        branches {
          nodes {
            name: "Nation AA {no-match}"
            condition { verbatim { op: EQUAL name: "some_field" value: "900" } }
          }
          nodes {
            name: "Nation BB {no-match}"
            condition {
              verbatim { op: EQUAL name: "some_field" value: "1000" }
            }
          }
        }
      )pb",
      &expected));
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, FormatTextFields) {
  // By default format text fields at the end.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "AA {keya}"
        comprehend {
          context {
            items { key: "keya" value: "AA" }
            items { key: "keyc" value: "CC" }
          }
        }
        branches {
          nodes { name: "BB {keyb}" }
          nodes { name: "CC {keyc}" }
        }
      )pb",
      &config));

  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "AA AA"
        branches {
          nodes { name: "BB {keyb}" }
          nodes { name: "CC CC" }
        }
      )pb",
      &expected));

  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, FormatTextFields_ContextOverride) {
  // Override context if an input map is supplied.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "AA {keya}"
        comprehend {
          context {
            items { key: "keya" value: "AA" }
            items { key: "keyc" value: "CC" }
          }
        }
        branches {
          nodes { name: "BB {keyb}" }
          nodes { name: "CC {keyc}" }
        }
      )pb",
      &config));
  ContextMap context_map;
  context_map["keya"] = "123";
  context_map["keyb"] = "456";

  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "AA 123"
        branches {
          nodes { name: "BB 456" }
          nodes { name: "CC CC" }
        }
      )pb",
      &expected));

  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, FormatTextFields_ExcludeFields) {
  // Test exclude_fields using the top level method.
  // Need to disable format in other nodes to not format again.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "AA {keya}"
        comprehend {
          context {
            items { key: "keya" value: "AA" }
            items { key: "keyb" value: "BB" }
            items { key: "keyc" value: "CC" }
          }
          methods {
            format_text_fields { exclude_fields: "branches.nodes.name" }
          }
          # Test exclude_fields using the top level method. Don't do it again.
          dont_apply_format_text_fields: true
        }
        branches {
          nodes {
            comprehend {
              # Test exclude_fields using the method above. Don't do it again.
              dont_apply_format_text_fields: true
            }
            name: "BB {keyb}"
            branches {
              nodes {
                comprehend {
                  # Test exclude_fields using the method above. Don't do it
                  # again.
                  dont_apply_format_text_fields: true
                }
                name: "next {keyb}"
              }
            }
          }
          nodes {
            comprehend {
              # Test exclude_fields using the method above. Don't do it again.
              dont_apply_format_text_fields: true
            }
            name: "CC {keyc}"
            branches {
              nodes {
                comprehend {
                  # Test exclude_fields using the method above. Don't do it
                  # again.
                  dont_apply_format_text_fields: true
                }
                name: "next {keyc}"
              }
            }
          }
        }
      )pb",
      &config));
  // Format all text fields except for "branches.nodes.name"
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "AA AA"
        branches {
          nodes {
            name: "BB {keyb}"
            branches { nodes { name: "next BB" } }
          }
          nodes {
            name: "CC {keyc}"
            branches { nodes { name: "next CC" } }
          }
        }
      )pb",
      &expected));

  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, FormatTextFields_Default) {
  // By default apply FormatTextFields at the end.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "{v3}"
        comprehend {
          context { items { key: "v0" value: "0" } }
          methods {
            set_values {
              keys_to_assign_values: "v1"
              map_spec { verbatim { items { values: "{v0} 1 {v0}" } } }
            }
          }
          methods {
            set_values {
              keys_to_assign_values: "v2"
              map_spec { verbatim { items { values: "{v1} 2 {v1}" } } }
            }
          }
          methods {
            set_values {
              keys_to_assign_values: "v3"
              map_spec { verbatim { items { values: "{v2} 3 {v2}" } } }
            }
          }
        }
      )pb",
      &config));
  // v1 = 0 1 0
  // v2 = 0 1 0 2 0 1 0
  ModelNodeConfig expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "0 1 0 2 0 1 0 3 0 1 0 2 0 1 0"
      )pb",
      &expected));

  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(
      ModelNodeConfig result,
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(ComprehensionMethodTest, Error_ApplyWithBaseClass) {
  ComprehensionMethod method;
  ModelNodeConfig config;
  EXPECT_THAT(
      method.Apply(config).status(),
      StatusIs(absl::StatusCode::kInternal, "Cannot use baseclass for Apply"));
}

TEST(ComprehensionMethodTest, Error_MethodNotSet) {
  Comprehend::Method config;
  EXPECT_THAT(ComprehensionMethod::Build(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "must set method"));
}

TEST(ComprehensionMethodTest, Error_ForEach_EntityNotSet) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { methods { for_each {} } }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "must set entity"));
}

TEST(ComprehensionMethodTest, Error_ForEach_ValuesNotSet) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { methods { for_each { entity: "b" } } }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "must set values"));
}

TEST(ComprehensionMethodTest, Error_ForEach_ValuesError) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        branches {
          nodes {
            comprehend {
              methods {
                for_each {
                  entity: "b"
                  values: { from_csv { filename: "x" } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "Cannot open file"));
}

TEST(ComprehensionMethodTest, Error_ForEach_EntityInContext) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                for_each {
                  entity: "country"
                  values: { verbatim { items: "x" items: "y" } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
          .status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "already in context"));
}

TEST(ComprehensionMethodTest, Error_SetValues_NoKeysToAssign) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches { nodes { comprehend { methods { set_values {} } } } }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "at least 1 keys_to_assign_values"));
}

TEST(ComprehensionMethodTest, Error_SetValues_DuplicateKey) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                set_values {
                  key_to_retrieve_values: "aa"
                  keys_to_assign_values: "aa"
                  keys_to_assign_values: "bb"
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "key_to_retrieve_values cannot be in"));
}

TEST(ComprehensionMethodTest, Error_SetValues_NoKeyToRetrieve) {
  // key_to_retrieve_values is not defined.
  // The mapping should have exactly 1 key which is empty string.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                set_values {
                  keys_to_assign_values: "outkey1"
                  keys_to_assign_values: "outkey2"
                  map_spec { verbatim { items { key: "a" values: "b" } } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
          .status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "have exactly 1 key"));
}

TEST(ComprehensionMethodTest, Error_SetValues_KeyToAssignInContext) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                set_values {
                  keys_to_assign_values: "country"
                  map_spec { verbatim { items { values: "b" } } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
          .status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "already in context"));
}

TEST(ComprehensionMethodTest, Error_SetValues_KeyToRetrieveNotInContext) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                set_values {
                  key_to_retrieve_values: "inkey"
                  keys_to_assign_values: "outkey"
                  map_spec { verbatim { items { key: "a" values: "b" } } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "not in context"));
}

TEST(ComprehensionMethodTest, Error_SetValues_NotInMapping) {
  // The value retrieved is not a key in the mapping.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                set_values {
                  key_to_retrieve_values: "country"
                  keys_to_assign_values: "outkey"
                  map_spec { verbatim { items { key: "a" values: "b" } } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "not in mapping"));
}

TEST(ComprehensionMethodTest, Error_SetValues_ValueSizeMismatch) {
  // There are 2 values and 1 keys_to_assign_values.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                set_values {
                  key_to_retrieve_values: "country"
                  keys_to_assign_values: "outkey"
                  map_spec {
                    verbatim { items { key: "A" values: "b" values: "c" } }
                  }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "!= size of keys_to_assign_values"));
}

TEST(ComprehensionMethodTest, Error_Filter_ExpressionNotSet) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches { nodes { comprehend { methods { filter {} } } } }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
          .status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set expression"));
}

TEST(ComprehensionMethodTest, Error_Filter_InvalidExpression) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend { methods { filter { expression { equality {} } } } }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "must set left_key and right_key"));
}

TEST(ComprehensionMethodTest, Error_Filter_EvaluationError) {
  // Expression evaluation failure.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                filter {
                  expression { equality { left_key: "a" right_key: "b" } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Key is not found in context map"));
}

TEST(ComprehensionMethodTest, Error_ApplyIf_ConditionNotSet) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches { nodes { comprehend { methods { apply_if {} } } } }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
          .status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set condition"));
}

TEST(ComprehensionMethodTest, Error_ApplyIf_IfMethodNotSet) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend { methods { apply_if { condition { equality {} } } } }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(
      ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
          .status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set if_method"));
}

TEST(ComprehensionMethodTest, Error_ApplyIf_InvalidCondition) {
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                apply_if {
                  condition { equality {} }
                  if_method {}
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "must set left_key and right_key"));
}

TEST(ComprehensionMethodTest, Error_ApplyIf_EvaludationError) {
  // Condition evaluation error.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend { context { items { key: "country" value: "A" } } }
        branches {
          nodes {
            comprehend {
              methods {
                apply_if {
                  condition { equality { left_key: "a" right_key: "b" } }
                  if_method {}
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Key is not found in context map"));
}

TEST(ComprehensionMethodTest, Error_ApplyIf_InvalidIfMethod) {
  // condition = true, use if_method, but it is invalid.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend {
          context {
            items { key: "a" value: "A" }
            items { key: "b" value: "A" }
          }
        }
        branches {
          nodes {
            comprehend {
              methods {
                apply_if {
                  condition { equality { left_key: "a" right_key: "b" } }
                  if_method {}
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "must set method"));
}

TEST(ComprehensionMethodTest, Error_ApplyIf_InvalidElseMethod) {
  // condition = false, use else_method, but it is invalid.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend {
          context {
            items { key: "a" value: "A" }
            items { key: "b" value: "B" }
          }
        }
        branches {
          nodes {
            comprehend {
              methods {
                apply_if {
                  condition { equality { left_key: "a" right_key: "b" } }
                  if_method { format_text_fields {} }
                  else_method {}
                }
              }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "must set method"));
}

TEST(ComprehensionMethodTest, Error_NotSingleNode) {
  // After comprehension there should be exactly one node.
  ModelNodeConfig config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "a"
        comprehend {
          methods {
            for_each {
              entity: "a"
              values { verbatim { items: "b" items: "c" } }
            }
          }
        }
      )pb",
      &config));
  ContextMap context_map;
  EXPECT_THAT(ComprehensionMethod::ComprehendAndCleanModel(config, context_map)
                  .status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "exactly 1 node"));
}

}  // namespace
}  // namespace wfa_virtual_people
