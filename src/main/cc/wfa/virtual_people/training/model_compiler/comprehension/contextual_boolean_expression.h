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

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_CONTEXTUAL_BOOLEAN_EXPRESSION_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_CONTEXTUAL_BOOLEAN_EXPRESSION_H_

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "wfa/virtual_people/training/comprehend.pb.h"

namespace wfa_virtual_people {

// Comprehension context is added to model node via the model configuration
// and comprehension methods. Conceptually the context is a mapping from
// key-string to value-string.
// Comprehension methods can write auxiliary information to the context for
// other comprehensions to use. The context is inherited by the child nodes.
typedef absl::flat_hash_map<std::string, std::string> ContextMap;

// The baseclass for boolean expression computable from context.
class ContextualBooleanExpression {
 public:
  // Always use ContextualBooleanExpression::Build to get a
  // ContextualBooleanExpression object. Users should never call the factory
  // function or constructor of the derived class directly.
  static absl::StatusOr<std::unique_ptr<ContextualBooleanExpression>> Build(
      const Comprehend::ContextualBooleanExpression& config);

  ContextualBooleanExpression();
  virtual ~ContextualBooleanExpression() = default;

  ContextualBooleanExpression(const ContextualBooleanExpression&) = delete;
  ContextualBooleanExpression& operator=(const ContextualBooleanExpression&) =
      delete;

  // Evaluates the expression using @context_map.
  // Returns true if it evaluates to true, and returns false if it evaluates
  // to false.
  virtual absl::StatusOr<bool> Evaluate(const ContextMap& context_map) const;
};

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_CONTEXTUAL_BOOLEAN_EXPRESSION_H_
