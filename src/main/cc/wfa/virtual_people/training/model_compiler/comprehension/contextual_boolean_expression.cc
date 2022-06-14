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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "common_cpp/macros/macros.h"
#include "wfa/virtual_people/training/comprehend.pb.h"

namespace wfa_virtual_people {

namespace {

// Gets iterator for @key in @context_map.
// Raises error if @key is not found.
absl::StatusOr<ContextMap::const_iterator> GetIterator(
    const ContextMap& context_map, absl::string_view key) {
  auto iter = context_map.find(key);
  if (iter == context_map.end()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Key is not found in context map:", key));
  }
  return iter;
}

// Returns true if the two given keys have the same value.
class Equality : public ContextualBooleanExpression {
 public:
  static absl::StatusOr<std::unique_ptr<Equality>> Build(
      const Comprehend::ContextualBooleanExpression::Equality& config) {
    if (config.has_left_key() && config.has_right_key()) {
      return absl::make_unique<Equality>(config.left_key(), config.right_key());
    }

    return absl::InvalidArgumentError(
        absl::StrCat("Equality expression must set left_key and right_key.",
                     config.DebugString()));
  }

  explicit Equality(absl::string_view left_key, absl::string_view right_key)
      : left_key_(left_key), right_key_(right_key) {}

  absl::StatusOr<bool> Evaluate(const ContextMap& context_map) const {
    ASSIGN_OR_RETURN(ContextMap::const_iterator left_iter,
                     GetIterator(context_map, left_key_));
    ASSIGN_OR_RETURN(ContextMap::const_iterator right_iter,
                     GetIterator(context_map, right_key_));
    return left_iter->second == right_iter->second;
  }

 private:
  std::string left_key_;
  std::string right_key_;
};

// Returns negation of the given expression.
class NotExpression : public ContextualBooleanExpression {
 public:
  static absl::StatusOr<std::unique_ptr<NotExpression>> Build(
      const Comprehend::ContextualBooleanExpression::NotExpression& config) {
    if (!config.has_expression()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "NotExpression must set expression.", config.DebugString()));
    }

    ASSIGN_OR_RETURN(std::unique_ptr<ContextualBooleanExpression> expression,
                     ContextualBooleanExpression::Build(config.expression()));
    return absl::make_unique<NotExpression>(std::move(expression));
  }

  explicit NotExpression(
      std::unique_ptr<ContextualBooleanExpression> expression)
      : expression_(std::move(expression)) {}

  absl::StatusOr<bool> Evaluate(const ContextMap& context_map) const {
    ASSIGN_OR_RETURN(bool result, expression_->Evaluate(context_map));
    return !result;
  }

 private:
  std::unique_ptr<ContextualBooleanExpression> expression_;
};

// Returns true if all of the given expressions evaluate to true.
class AndExpression : public ContextualBooleanExpression {
 public:
  static absl::StatusOr<std::unique_ptr<AndExpression>> Build(
      const Comprehend::ContextualBooleanExpression::AndExpression& config) {
    std::vector<std::unique_ptr<ContextualBooleanExpression>> expressions;
    for (const Comprehend::ContextualBooleanExpression& expression :
         config.expressions()) {
      expressions.emplace_back();
      ASSIGN_OR_RETURN(expressions.back(),
                       ContextualBooleanExpression::Build(expression));
      if (!expressions.back()) {
        return absl::InternalError(absl::StrCat(
            "Failed to build ContextualBooleanExpression with config: ",
            expression.DebugString()));
      }
    }

    return absl::make_unique<AndExpression>(std::move(expressions));
  }

  explicit AndExpression(
      std::vector<std::unique_ptr<ContextualBooleanExpression>> expressions)
      : expressions_(std::move(expressions)) {}

  // Empty list returns true.
  absl::StatusOr<bool> Evaluate(const ContextMap& context_map) const {
    bool all_true = true;
    for (auto& exp : expressions_) {
      ASSIGN_OR_RETURN(bool result, exp->Evaluate(context_map));
      all_true &= result;
    }

    return all_true;
  }

 private:
  std::vector<std::unique_ptr<ContextualBooleanExpression>> expressions_;
};

// Returns true if any of the given expressions evaluate to true.
class OrExpression : public ContextualBooleanExpression {
 public:
  static absl::StatusOr<std::unique_ptr<OrExpression>> Build(
      const Comprehend::ContextualBooleanExpression::OrExpression& config) {
    std::vector<std::unique_ptr<ContextualBooleanExpression>> expressions;
    for (const Comprehend::ContextualBooleanExpression& expression :
         config.expressions()) {
      expressions.emplace_back();
      ASSIGN_OR_RETURN(expressions.back(),
                       ContextualBooleanExpression::Build(expression));
      if (!expressions.back()) {
        return absl::InternalError(absl::StrCat(
            "Failed to build ContextualBooleanExpression with config: ",
            expression.DebugString()));
      }
    }

    return absl::make_unique<OrExpression>(std::move(expressions));
  }

  explicit OrExpression(
      std::vector<std::unique_ptr<ContextualBooleanExpression>> expressions)
      : expressions_(std::move(expressions)) {}

  // Empty list returns false.
  absl::StatusOr<bool> Evaluate(const ContextMap& context_map) const {
    bool any_true = false;
    for (auto& exp : expressions_) {
      ASSIGN_OR_RETURN(bool result, exp->Evaluate(context_map));
      any_true |= result;
    }

    return any_true;
  }

 private:
  std::vector<std::unique_ptr<ContextualBooleanExpression>> expressions_;
};

}  // namespace

absl::StatusOr<std::unique_ptr<ContextualBooleanExpression>>
ContextualBooleanExpression::Build(
    const Comprehend::ContextualBooleanExpression& config) {
  switch (config.expression_case()) {
    case Comprehend::ContextualBooleanExpression::ExpressionCase::kEquality:
      return Equality::Build(config.equality());

    case Comprehend::ContextualBooleanExpression::ExpressionCase::
        kNotExpression:
      return NotExpression::Build(config.not_expression());

    case Comprehend::ContextualBooleanExpression::ExpressionCase::
        kAndExpression:
      return AndExpression::Build(config.and_expression());

    case Comprehend::ContextualBooleanExpression::ExpressionCase::kOrExpression:
      return OrExpression::Build(config.or_expression());

    default:
      // No expression is set.
      return absl::InvalidArgumentError(
          "ContextualBooleanExpression must set expression.");
  }
}

ContextualBooleanExpression::ContextualBooleanExpression() {}

absl::StatusOr<bool> ContextualBooleanExpression::Evaluate(
    const ContextMap& context_map) const {
  return absl::InternalError(
      "ContextualBooleanExpression: Cannot use baseclass for evaluation.");
}

}  // namespace wfa_virtual_people
