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

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_METHOD_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_METHOD_H_

#include <vector>
#include <memory>

#include "absl/status/statusor.h"
#include "wfa/virtual_people/training/comprehend.pb.h"
#include "wfa/virtual_people/training/model_compiler/comprehension/contextual_boolean_expression.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {

// The baseclass for comprehension methods.
class ComprehensionMethod {
 public:
  // Always use ComprehensionMethod::Build to get a
  // ComprehensionMethod object. Users should never call the factory
  // function or constructor of the derived class directly.
  static absl::StatusOr<std::unique_ptr<ComprehensionMethod>> Build(
      const Comprehend::Method& config);

  // Comprehend @node_config and return the comprehended config.
  // May override context in @node_config(including children) with @context_map.
  static absl::StatusOr<ModelNodeConfig> ComprehendAndCleanModel(
      ModelNodeConfig& node_config, const ContextMap& context_map);

  ComprehensionMethod();
  virtual ~ComprehensionMethod() = default;

  ComprehensionMethod(const ComprehensionMethod&) = delete;
  ComprehensionMethod& operator=(const ComprehensionMethod&) = delete;

  // Apply comprehension method to @node_config to produce a list of nodes.
  virtual absl::StatusOr<std::vector<ModelNodeConfig>> Apply(
      ModelNodeConfig& node_config) const;
};

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_METHOD_H_
