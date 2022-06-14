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

#include "wfa/virtual_people/training/model_compiler/comprehension/contextual_boolean_expression.h"

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common_cpp/testing/status_macros.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/training/comprehend.pb.h"

namespace wfa_virtual_people {
namespace {

using ::wfa::StatusIs;

TEST(ContextualBooleanExpressionTest, EvaluateWithBaseClass) {
  ContextualBooleanExpression expression;
  ContextMap context_map;
  EXPECT_THAT(expression.Evaluate(context_map).status(),
              StatusIs(absl::StatusCode::kInternal,
                       "Cannot use baseclass for evaluation"));
}

TEST(ContextualBooleanExpressionTest, ExpressionNotSet) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
      )pb",
      &config));
  EXPECT_THAT(
      ContextualBooleanExpression::Build(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set expression"));
}

TEST(ContextualBooleanExpressionTest, Equality_KeyNotSet) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        equality {

        }
      )pb",
      &config));
  EXPECT_THAT(ContextualBooleanExpression::Build(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "must set left_key and right_key"));
}

TEST(ContextualBooleanExpressionTest, Equality_KeyNotInMap) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        equality { left_key: "a" right_key: "b" }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  ContextMap context_map;
  EXPECT_THAT(expression->Evaluate(context_map).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "Key is not found"));
}

TEST(ContextualBooleanExpressionTest, Equality_ReturnTrue) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        equality { left_key: "a" right_key: "b" }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "123";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_TRUE(res);
}

TEST(ContextualBooleanExpressionTest, Equality_ReturnFalse) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        equality { left_key: "a" right_key: "b" }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "456";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_FALSE(res);
}

TEST(ContextualBooleanExpressionTest, Equality_SameKeyForLeftAndRight) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        equality { left_key: "a" right_key: "a" }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  ContextMap context_map;
  context_map["a"] = "123";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_TRUE(res);
}

TEST(ContextualBooleanExpressionTest, Equality_CheckValueNotKeyName) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        equality { left_key: "a" right_key: "b" }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // The equality check is on value, not on key name.
  ContextMap context_map;
  context_map["a"] = "b";
  context_map["b"] = "a";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_FALSE(res);
}

TEST(ContextualBooleanExpressionTest, NotExpression_ExpressionNotSet) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        not_expression {}
      )pb",
      &config));
  EXPECT_THAT(
      ContextualBooleanExpression::Build(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set expression"));
}

TEST(ContextualBooleanExpressionTest, NotExpression_ExpressionBuildError) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        not_expression { expression { equality {} } }
      )pb",
      &config));
  EXPECT_THAT(ContextualBooleanExpression::Build(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "must set left_key and right_key"));
}

TEST(ContextualBooleanExpressionTest, NotExpression_ReturnsTrue) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        not_expression {
          expression { equality { left_key: "a" right_key: "b" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return negation of expression.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "456";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_TRUE(res);
}

TEST(ContextualBooleanExpressionTest, NotExpression_ReturnsFalse) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        not_expression {
          expression { equality { left_key: "a" right_key: "b" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return negation of expression.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "123";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_FALSE(res);
}

TEST(ContextualBooleanExpressionTest, AndExpression_EmptyExpressions) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        and_expression {}
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Empty list returns true for AndExpression.
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_TRUE(res);
}

TEST(ContextualBooleanExpressionTest, AndExpression_ExpressionBuildError) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        and_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" } }
        }
      )pb",
      &config));
  EXPECT_THAT(ContextualBooleanExpression::Build(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "must set left_key and right_key"));
}

TEST(ContextualBooleanExpressionTest, AndExpression_HasEvaluationError) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        and_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" right_key: "d" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return error if any expression has error.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "456";
  EXPECT_THAT(expression->Evaluate(context_map).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "Key is not found"));
}

TEST(ContextualBooleanExpressionTest, AndExpression_ReturnTrue) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        and_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" right_key: "d" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return true if all true.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "123";
  context_map["c"] = "456";
  context_map["d"] = "456";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_TRUE(res);
}

TEST(ContextualBooleanExpressionTest, AndExpression_ReturnFalse) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        and_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" right_key: "d" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return false if not all true.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "123";
  context_map["c"] = "123";
  context_map["d"] = "456";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_FALSE(res);
}

TEST(ContextualBooleanExpressionTest, OrExpression_EmptyExpressions) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        or_expression {}
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Empty list returns false for OrExpression.
  ContextMap context_map;
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_FALSE(res);
}

TEST(ContextualBooleanExpressionTest, OrExpression_ExpressionBuildError) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        or_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" } }
        }
      )pb",
      &config));
  EXPECT_THAT(ContextualBooleanExpression::Build(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "must set left_key and right_key"));
}

TEST(ContextualBooleanExpressionTest, OrExpression_HasEvaluationError) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        or_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" right_key: "d" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return error if any expression has error.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "123";
  EXPECT_THAT(expression->Evaluate(context_map).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "Key is not found"));
}

TEST(ContextualBooleanExpressionTest, OrExpression_ReturnTrue) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        or_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" right_key: "d" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return true if any true.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "123";
  context_map["c"] = "789";
  context_map["d"] = "456";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_TRUE(res);
}

TEST(ContextualBooleanExpressionTest, OrExpression_ReturnFalse) {
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        or_expression {
          expressions { equality { left_key: "a" right_key: "b" } }
          expressions { equality { left_key: "c" right_key: "d" } }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // Return false if all false.
  ContextMap context_map;
  context_map["a"] = "123";
  context_map["b"] = "456";
  context_map["c"] = "789";
  context_map["d"] = "456";
  ASSERT_OK_AND_ASSIGN(bool res, expression->Evaluate(context_map));
  EXPECT_FALSE(res);
}

TEST(ContextualBooleanExpressionTest, NestedExpression) {
  // expression: a == b && !(c == d)
  Comprehend::ContextualBooleanExpression config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        and_expression {
          expressions {
            or_expression {
              expressions { equality { left_key: "a" right_key: "b" } }
            }
          }
          expressions {
            or_expression {
              expressions {
                not_expression {
                  expression { equality { left_key: "c" right_key: "d" } }
                }
              }
            }
          }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<ContextualBooleanExpression> expression,
                       ContextualBooleanExpression::Build(config));

  // True
  ContextMap context_map;
  bool res = false;

  context_map["a"] = "123";
  context_map["b"] = "123";
  context_map["c"] = "789";
  context_map["d"] = "456";
  ASSERT_OK_AND_ASSIGN(res, expression->Evaluate(context_map));
  EXPECT_TRUE(res);

  // False
  context_map["a"] = "456";
  ASSERT_OK_AND_ASSIGN(res, expression->Evaluate(context_map));
  EXPECT_FALSE(res);
}

}  // namespace
}  // namespace wfa_virtual_people
