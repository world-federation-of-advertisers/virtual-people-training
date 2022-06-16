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

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_COMPREHENSION_METHOD_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_COMPREHENSION_METHOD_H_

#include <memory>
#include <vector>

#include "absl/status/statusor.h"
#include "wfa/virtual_people/training/comprehend.pb.h"
#include "wfa/virtual_people/training/model_compiler/comprehension/contextual_boolean_expression.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {

// Each ModelNodeConfig has a context ModelNodeConfig.comprehend.context,
// which is a string-to-string mapping.
//
// Apply a comprehension method to a MondelNodeConfig to produce a list of
// ModelNodeConfig, using context of the input node.
// - Each node in the output is a copy of the input node, with some update.
// - The returned list can be empty.
//
// The following methods are supported:
// - ForEach: Given entity(i.e. key), for each value from a list, a node is
//   created and "entity=value" is added into the new node's context and all its
//   descendants.
// - SetValues: Assigns values to keys in new node's context, and returns a list
//   containing the new node.
// - Filter: Returns a list containing the new node if a condition evaluates
//   to true, otherwise returns an empty list.
// - ApplyIf: If a condition evaluates to true, apply method1 to the new node,
//   otherwise apply method2 (could be no-op). Returns a list containing the new
//   node.
// - FormatTextFields: Format all text fields in the new node, using context
//   as the formatting dictionary. Returns a list containing the new node. By
//   default, this method is always applied as the last comprehension, and child
//   nodes are excluded from formatting.
// See comprehend.proto for formal definition.
//
// Examples:
// Note that by default FormatTextFields is applied as the last method.
// The results below take this into account.
//
// input node = {name: "{a} {x} {y}"}
// with context map
//   "a" -> "v1"
//   "b" -> "v1"
//   "c" -> "v2"
//   "d" -> "v3"
//
// FormatTextFields:
// Returns a list of one node:
// {name: "v1 {x} {y}"}
// "a" is in the context, but "x" and "y" are not.
//
// ForEach: entity = "x", values = ["xa", "xb"]
// Returns a list of two nodes:
// {name: "v1 xa {y}"}, with "x" -> "xa" added to context
// {name: "v1 xb {y}"}, with "x" -> "xb" added to context
//
// SetValues: "x" -> "xa", "y" -> "ya"
// Returns a list of one node:
// {name: "v1 xa ya"}, with "x" -> "xa" and "y" -> "ya" added to context
//
// Filter: condition: a == b (these are key names)
// condition = true
// Returns a list of one node:
// {name: "v1 {x} {y}"}
//
// Filter: condition: c == d (these are key names)
// condition = false
// Returns empty list
//
// ApplyIf: condition: a == b (these are key names)
// if_method: SetValues: "x" -> "xa"
// else_method: SetValues: "y" -> "ya"
// condition = true
// Returns a list of one node:
// {name: "v1 xa {y}"}, with "x" -> "xa" added to context
//
// ApplyIf: condition: c == d (these are key names)
// if_method: SetValues: "x" -> "xa"
// else_method: SetValues: "y" -> "ya"
// condition = false
// Returns a list of one node:
// {name: "v1 {x} ya"}, with "y" -> "ya" added to context

// The baseclass for comprehension methods.
class ComprehensionMethod {
 public:
  // Comprehend @node_config and return the comprehended config.
  // May override context in @node_config(including children) with @context_map.
  static absl::StatusOr<ModelNodeConfig> ComprehendAndCleanModel(
      ModelNodeConfig& node_config, const ContextMap& context_map);

  // Always use ComprehensionMethod::Build to get a
  // ComprehensionMethod object. Users should never call the factory
  // function or constructor of the derived class directly.
  static absl::StatusOr<std::unique_ptr<ComprehensionMethod>> Build(
      const Comprehend::Method& config);

  ComprehensionMethod();
  virtual ~ComprehensionMethod() = default;

  ComprehensionMethod(const ComprehensionMethod&) = delete;
  ComprehensionMethod& operator=(const ComprehensionMethod&) = delete;

  // Apply comprehension method to @node_config to produce a list of nodes.
  virtual absl::StatusOr<std::vector<ModelNodeConfig>> Apply(
      ModelNodeConfig& node_config) const;
};

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_COMPREHENSION_METHOD_H_
