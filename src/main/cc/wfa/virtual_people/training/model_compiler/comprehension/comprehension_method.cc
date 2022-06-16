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

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "common_cpp/macros/macros.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "wfa/virtual_people/training/comprehend.pb.h"
#include "wfa/virtual_people/training/model_compiler/comprehension/contextual_boolean_expression.h"
#include "wfa/virtual_people/training/model_compiler/comprehension/spec_util.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {

namespace {

// Forward declaration.
absl::Status FormatStringsInMessage(
    google::protobuf::Message& message, const ContextMap& context_map,
    const std::vector<std::string>& exclude_fields);
absl::StatusOr<std::vector<ModelNodeConfig>> ComprehendRecursively(
    ModelNodeConfig& node_config);

// Parse context config into a map.
// If add_braces = true, surround key with {}.
ContextMap ContextAsMap(const Comprehend::Context& config, bool add_braces) {
  ContextMap context_map;
  for (Comprehend::Context::KeyValue kv : config.items()) {
    if (add_braces) {
      context_map[absl::StrCat("{", kv.key(), "}")] = kv.value();
    } else {
      context_map[kv.key()] = kv.value();
    }
  }
  return context_map;
}

// Use context map to create a Context proto.
Comprehend::Context MapAsContext(const ContextMap& context_map) {
  Comprehend::Context context;
  for (auto& key_value : context_map) {
    auto item = context.add_items();
    item->set_key(key_value.first);
    item->set_value(key_value.second);
  }
  return context;
}

// Compute a set of fields to exclude for child message.
// For example: if the fields we exclude in the parent are
//     ['a', 'a.w', 'b.x', 'b.y', 'b.z.t', 'c']
// then for child message 'b' we will exclude
//     ['x', 'y', 'z.t']
std::vector<std::string> GetFieldsToExcludeInSubMessage(
    const std::vector<std::string>& parent_exclude_fields,
    absl::string_view child_name) {
  std::vector<std::string> result;
  for (const std::string& field : parent_exclude_fields) {
    std::vector<std::string> path = absl::StrSplit(field, '.');
    if (path.size() > 1 && path[0] == child_name) {
      uint64_t pos = field.find('.', 0);
      result.emplace_back(field.substr(pos + 1));
    }
  }
  return result;
}

// Copy context to all child nodes.
void PassContextToChildren(ModelNodeConfig& node_config) {
  if (!node_config.has_branches()) {
    return;
  }

  for (auto& child : *node_config.mutable_branches()->mutable_nodes()) {
    child.mutable_comprehend()->mutable_context()->MergeFrom(
        node_config.comprehend().context());
  }
}

// Extract the next comprehension method.
std::unique_ptr<Comprehend::Method> ExtractInnerMethod(
    ModelNodeConfig& node_config) {
  // Take the 0-th method and remove from config.
  if (node_config.has_comprehend() &&
      node_config.comprehend().methods().size() > 0) {
    auto method = absl::make_unique<Comprehend::Method>(
        node_config.comprehend().methods(0));
    node_config.mutable_comprehend()->mutable_methods()->erase(
        node_config.comprehend().methods().begin());
    return method;
  }

  // Have applied all methods in node config.
  // By default, format text fields.
  if (!node_config.comprehend().dont_apply_format_text_fields()) {
    auto method = absl::make_unique<Comprehend::Method>();
    // Exclude child nodes. They will be processed later.
    method->mutable_format_text_fields()->add_exclude_fields("branches");
    // Only need to do it once.
    node_config.mutable_comprehend()->set_dont_apply_format_text_fields(true);
    return method;
  }

  return std::unique_ptr<Comprehend::Method>();
}

// Apply the comprehension method on node to produce a list of nodes.
absl::StatusOr<std::vector<ModelNodeConfig>> ComprehendNode(
    ModelNodeConfig& node_config, Comprehend::Method& method_config) {
  // Format method config with context.
  bool add_braces = true;                   // For string format.
  std::vector<std::string> exclude_fields;  // No field to exclude here.
  ContextMap context_map =
      ContextAsMap(node_config.comprehend().context(), add_braces);
  RETURN_IF_ERROR(
      FormatStringsInMessage(method_config, context_map, exclude_fields));

  // Apply the method.
  ASSIGN_OR_RETURN(std::unique_ptr<ComprehensionMethod> method,
                   ComprehensionMethod::Build(method_config));
  if (method == nullptr) {
    return absl::InternalError(
        absl::StrCat("ComprehensionMethod::Build should never return null.",
                     method_config.DebugString()));
  }

  return method->Apply(node_config);
}

// Comprehend a list of nodes and produce a new list.
// The vector concat is not efficient, but we expect that the total
// number of nodes is small and performance is not critical.
absl::StatusOr<std::vector<ModelNodeConfig>> ComprehendNodesRecursively(
    std::vector<ModelNodeConfig>& nodes) {
  std::vector<ModelNodeConfig> result;
  for (ModelNodeConfig& node : nodes) {
    ASSIGN_OR_RETURN(std::vector<ModelNodeConfig> current,
                     ComprehendRecursively(node));
    result.insert(result.end(), current.begin(), current.end());
  }
  return result;
}

// Comprehend a node and its children.
absl::StatusOr<std::vector<ModelNodeConfig>> ComprehendRecursively(
    ModelNodeConfig& node_config) {
  std::unique_ptr<Comprehend::Method> method_config =
      ExtractInnerMethod(node_config);
  if (method_config == nullptr) {
    // No more comprehension method here. Comprehend child nodes.
    if (node_config.has_branches()) {
      PassContextToChildren(node_config);
      std::vector<ModelNodeConfig> children(
          node_config.branches().nodes().begin(),
          node_config.branches().nodes().end());
      ASSIGN_OR_RETURN(std::vector<ModelNodeConfig> result,
                       ComprehendNodesRecursively(children));
      // Replace child nodes.
      node_config.mutable_branches()->mutable_nodes()->Assign(result.begin(),
                                                              result.end());
    }
    return std::vector<ModelNodeConfig>{node_config};
  } else {
    ASSIGN_OR_RETURN(std::vector<ModelNodeConfig> result,
                     ComprehendNode(node_config, *method_config));
    // Empty list is a valid result of comprehension.
    return ComprehendNodesRecursively(result);
  }
}

// Format string fields in message recursively using context map.
// Skip fields in exclude_fields.
absl::Status FormatStringsInMessage(
    google::protobuf::Message& message, const ContextMap& context_map,
    const std::vector<std::string>& exclude_fields) {
  const google::protobuf::Reflection* reflection = message.GetReflection();
  std::vector<const google::protobuf::FieldDescriptor*> field_descriptors;
  reflection->ListFields(message, &field_descriptors);

  for (const google::protobuf::FieldDescriptor* field_descriptor :
       field_descriptors) {
    const std::string& field_name = field_descriptor->name();
    if (std::find(exclude_fields.begin(), exclude_fields.end(),
                  field_descriptor->name()) != exclude_fields.end()) {
      continue;
    }

    std::vector<std::string> submessage_fields_to_exclude;
    if (field_descriptor->cpp_type() ==
        google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      submessage_fields_to_exclude =
          GetFieldsToExcludeInSubMessage(exclude_fields, field_name);
    }

    if (field_descriptor->is_optional()) {
      if (field_descriptor->cpp_type() ==
          google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
        std::string current_value =
            reflection->GetString(message, field_descriptor);
        reflection->SetString(&message, field_descriptor,
                              absl::StrReplaceAll(current_value, context_map));
      } else if (field_descriptor->cpp_type() ==
                 google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        RETURN_IF_ERROR(FormatStringsInMessage(
            *reflection->MutableMessage(&message, field_descriptor),
            context_map, submessage_fields_to_exclude));
      } else {
        // Do nothing for other fields.
      }
    } else if (field_descriptor->is_repeated()) {
      if (field_descriptor->cpp_type() ==
          google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
        auto field_ref = reflection->GetMutableRepeatedFieldRef<std::string>(
            &message, field_descriptor);
        for (int i = 0; i < field_ref.size(); ++i) {
          const std::string& current_value = field_ref.Get(i);
          field_ref.Set(i, absl::StrReplaceAll(current_value, context_map));
        }
      } else if (field_descriptor->cpp_type() ==
                 google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        int size = reflection
                       ->GetMutableRepeatedFieldRef<google::protobuf::Message>(
                           &message, field_descriptor)
                       .size();
        for (int i = 0; i < size; ++i) {
          auto submessage =
              reflection->MutableRepeatedMessage(&message, field_descriptor, i);
          RETURN_IF_ERROR(FormatStringsInMessage(*submessage, context_map,
                                                 submessage_fields_to_exclude));
        }
      } else {
        // Do nothing for other fields.
      }
    } else {
      return absl::InternalError(absl::StrCat("Unsupported type.", field_name));
    }
  }

  return absl::OkStatus();
}

// Use this to comprehend the top-level node, i.e. the root node.
absl::StatusOr<ModelNodeConfig> ComprehendModel(ModelNodeConfig& node_config) {
  ASSIGN_OR_RETURN(std::vector<ModelNodeConfig> res,
                   ComprehendRecursively(node_config));
  if (res.size() != 1) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Expects exactly 1 node after comprehending the root node. Get ",
        res.size()));
  }
  return res[0];
}

// Clear comprehension recursively.
void ClearComprehension(ModelNodeConfig& node_config) {
  node_config.clear_comprehend();
  if (node_config.has_branches()) {
    for (auto& child : *node_config.mutable_branches()->mutable_nodes()) {
      ClearComprehension(child);
    }
  }
}

// Create a list of nodes, one for each value in the list.
class ForEach : public ComprehensionMethod {
 public:
  static absl::StatusOr<std::unique_ptr<ForEach>> Build(
      const Comprehend::Method::ForEach& config) {
    if (!config.has_entity()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "ForEach method must set entity.", config.DebugString()));
    }

    if (!config.has_values()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "ForEach method must set values.", config.DebugString()));
    }

    ASSIGN_OR_RETURN(std::vector<std::string> values,
                     ReadListFromSpec(config.values()));
    return absl::make_unique<ForEach>(config.entity(), std::move(values));
  }

  explicit ForEach(absl::string_view entity, std::vector<std::string> values)
      : entity_(entity), values_(std::move(values)) {}

  absl::StatusOr<std::vector<ModelNodeConfig>> Apply(
      ModelNodeConfig& node_config) const {
    bool add_braces = false;  // Not for string format.
    ContextMap context =
        ContextAsMap(node_config.comprehend().context(), add_braces);
    if (context.find(entity_) != context.end()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "ForEach method entity is already in context map.", entity_));
    }

    std::vector<ModelNodeConfig> result;
    for (const std::string& value : values_) {
      result.emplace_back(node_config);

      // Add entity to context.
      Comprehend::Context::KeyValue& keyvalue =
          *result.back().mutable_comprehend()->mutable_context()->add_items();
      keyvalue.set_key(entity_);
      keyvalue.set_value(value);
    }
    return result;
  }

 private:
  std::string entity_;
  std::vector<std::string> values_;
};

// Set values in the context.
class SetValues : public ComprehensionMethod {
 public:
  static absl::StatusOr<std::unique_ptr<SetValues>> Build(
      const Comprehend::Method::SetValues& config) {
    if (config.keys_to_assign_values_size() == 0) {
      return absl::InvalidArgumentError(
          absl::StrCat("SetValues must have at least 1 keys_to_assign_values.",
                       config.DebugString()));
    }

    std::string key_to_retrieve_values(config.key_to_retrieve_values());
    std::vector<std::string> keys_to_assign_values(
        config.keys_to_assign_values().begin(),
        config.keys_to_assign_values().end());
    if (std::find(keys_to_assign_values.begin(), keys_to_assign_values.end(),
                  key_to_retrieve_values) != keys_to_assign_values.end()) {
      return absl::InvalidArgumentError(
          absl::StrCat("SetValues key_to_retrieve_values cannot be in "
                       "keys_to_assign_values.",
                       config.DebugString()));
    }

    ASSIGN_OR_RETURN(StringToStringsMap mapping,
                     ReadMapFromSpec(config.map_spec()));

    // It is possible key_to_retrieve_values is not explicitly defined,
    // therefore uses default empty string. In this case mapping should
    // have exactly 1 key = "".
    if (key_to_retrieve_values.empty()) {
      if (mapping.size() != 1 || mapping.find("") == mapping.end()) {
        return absl::InvalidArgumentError(absl::StrCat(
            "SetValues key_to_retrieve_values is not defined. The mapping "
            "should have exactly 1 key which is empty string.",
            config.DebugString()));
      }
    }

    return absl::make_unique<SetValues>(std::move(key_to_retrieve_values),
                                        std::move(keys_to_assign_values),
                                        std::move(mapping));
  }

  explicit SetValues(std::string key_to_retrieve_values,
                     std::vector<std::string> keys_to_assign_values,
                     StringToStringsMap mapping)
      : key_to_retrieve_values_(std::move(key_to_retrieve_values)),
        keys_to_assign_values_(std::move(keys_to_assign_values)),
        mapping_(std::move(mapping)) {}

  absl::StatusOr<std::vector<ModelNodeConfig>> Apply(
      ModelNodeConfig& node_config) const {
    ModelNodeConfig new_config(node_config);
    bool add_braces = false;  // Not for string format.
    ContextMap context_map =
        ContextAsMap(node_config.comprehend().context(), add_braces);

    for (std::string key : keys_to_assign_values_) {
      if (context_map.find(key) != context_map.end()) {
        return absl::InvalidArgumentError(absl::StrCat(
            "SetValues keys_to_assign_values_ ", key, " already in context."));
      }
    }

    std::string target;
    if (!key_to_retrieve_values_.empty()) {
      if (context_map.find(key_to_retrieve_values_) == context_map.end()) {
        return absl::InvalidArgumentError(
            absl::StrCat("SetValues key_to_retrieve_values ",
                         key_to_retrieve_values_, " not in context."));
      }
      target = context_map[key_to_retrieve_values_];
    }

    if (mapping_.find(target) == mapping_.end()) {
      return absl::InvalidArgumentError(
          absl::StrCat("SetValues target ", target, " not in mapping."));
    }

    const std::vector<std::string>& values = mapping_.at(target);
    if (values.size() != keys_to_assign_values_.size()) {
      return absl::InvalidArgumentError(
          absl::StrCat("SetValues value size for ", target,
                       " != size of keys_to_assign_values."));
    }

    for (int i = 0; i < values.size(); ++i) {
      auto new_item =
          new_config.mutable_comprehend()->mutable_context()->add_items();
      new_item->set_key(keys_to_assign_values_[i]);
      new_item->set_value(values[i]);
    }

    return std::vector<ModelNodeConfig>{new_config};
  }

 private:
  std::string key_to_retrieve_values_;
  std::vector<std::string> keys_to_assign_values_;
  StringToStringsMap mapping_;
};

// Format text fields in the model config.
class FormatTextFields : public ComprehensionMethod {
 public:
  static absl::StatusOr<std::unique_ptr<FormatTextFields>> Build(
      const Comprehend::Method::FormatTextFields& config) {
    std::vector<std::string> exclude_fields(config.exclude_fields().begin(),
                                            config.exclude_fields().end());
    return absl::make_unique<FormatTextFields>(std::move(exclude_fields));
  }

  explicit FormatTextFields(std::vector<std::string> exclude_fields)
      : exclude_fields_(std::move(exclude_fields)) {}

  absl::StatusOr<std::vector<ModelNodeConfig>> Apply(
      ModelNodeConfig& node_config) const {
    ModelNodeConfig new_config(node_config);
    bool add_braces = true;  // For string format.
    ContextMap context_map =
        ContextAsMap(node_config.comprehend().context(), add_braces);
    RETURN_IF_ERROR(
        FormatStringsInMessage(new_config, context_map, exclude_fields_));
    return std::vector<ModelNodeConfig>{new_config};
  }

 private:
  std::vector<std::string> exclude_fields_;
};

// Filter out if the given boolean expression evaluates to false.
class Filter : public ComprehensionMethod {
 public:
  static absl::StatusOr<std::unique_ptr<Filter>> Build(
      const Comprehend::Method::Filter& config) {
    if (!config.has_expression()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Filter method must set expression.", config.DebugString()));
    }

    ASSIGN_OR_RETURN(std::unique_ptr<ContextualBooleanExpression> expression,
                     ContextualBooleanExpression::Build(config.expression()));
    return absl::make_unique<Filter>(std::move(expression));
  }

  explicit Filter(std::unique_ptr<ContextualBooleanExpression> expression)
      : expression_(std::move(expression)) {}

  absl::StatusOr<std::vector<ModelNodeConfig>> Apply(
      ModelNodeConfig& node_config) const {
    if (expression_ == nullptr) {
      return absl::InvalidArgumentError(
          absl::StrCat("Filter.Apply() is called but expression is null."));
    }

    bool add_braces = false;  // Not for string format.
    ContextMap context =
        ContextAsMap(node_config.comprehend().context(), add_braces);
    ASSIGN_OR_RETURN(bool eval_result, expression_->Evaluate(context));

    std::vector<ModelNodeConfig> result;
    if (eval_result) {
      result.emplace_back(node_config);
    }
    return result;
  }

 private:
  std::unique_ptr<ContextualBooleanExpression> expression_;
};

// Apply one or the other method depending on a condition.
class ApplyIf : public ComprehensionMethod {
 public:
  static absl::StatusOr<std::unique_ptr<ApplyIf>> Build(
      const Comprehend::Method::ApplyIf& config) {
    if (!config.has_condition()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "ApplyIf method must set condition.", config.DebugString()));
    }
    if (!config.has_if_method()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "ApplyIf method must set if_method.", config.DebugString()));
    }

    ASSIGN_OR_RETURN(std::unique_ptr<ContextualBooleanExpression> condition,
                     ContextualBooleanExpression::Build(config.condition()));
    return absl::make_unique<ApplyIf>(std::move(condition), config);
  }

  explicit ApplyIf(std::unique_ptr<ContextualBooleanExpression> condition,
                   const Comprehend::Method::ApplyIf& config)
      : condition_(std::move(condition)),
        if_method_(config.if_method()),
        else_method_(config.else_method()),
        has_else_method_(config.has_else_method()) {}

  absl::StatusOr<std::vector<ModelNodeConfig>> Apply(
      ModelNodeConfig& node_config) const {
    if (condition_ == nullptr) {
      return absl::InvalidArgumentError(
          absl::StrCat("ApplyIf.Apply() is called but condition is null."));
    }

    bool add_braces = false;  // Not for string format.
    ContextMap context =
        ContextAsMap(node_config.comprehend().context(), add_braces);
    ASSIGN_OR_RETURN(bool eval_result, condition_->Evaluate(context));

    std::vector<ModelNodeConfig> result{node_config};
    ModelNodeConfig& new_config = result.back();
    // Insert the appropriate method at the beginning.
    new_config.mutable_comprehend()->clear_methods();
    if (eval_result) {
      *new_config.mutable_comprehend()->mutable_methods()->Add() = if_method_;
    } else if (has_else_method_) {
      *new_config.mutable_comprehend()->mutable_methods()->Add() = else_method_;
    }

    new_config.mutable_comprehend()->mutable_methods()->Add(
        node_config.comprehend().methods().begin(),
        node_config.comprehend().methods().end());
    return result;
  }

 private:
  std::unique_ptr<ContextualBooleanExpression> condition_;
  Comprehend::Method if_method_;
  Comprehend::Method else_method_;
  bool has_else_method_;
};

}  // namespace

absl::StatusOr<std::unique_ptr<ComprehensionMethod>> ComprehensionMethod::Build(
    const Comprehend::Method& config) {
  switch (config.method_case()) {
    case Comprehend::Method::MethodCase::kForEach:
      return ForEach::Build(config.for_each());

    case Comprehend::Method::MethodCase::kSetValues:
      return SetValues::Build(config.set_values());

    case Comprehend::Method::MethodCase::kFilter:
      return Filter::Build(config.filter());

    case Comprehend::Method::MethodCase::kFormatTextFields:
      return FormatTextFields::Build(config.format_text_fields());

    case Comprehend::Method::MethodCase::kApplyIf:
      return ApplyIf::Build(config.apply_if());

    default:
      // No method is set.
      return absl::InvalidArgumentError("Comprehend Method must set method.");
  }
}

absl::StatusOr<ModelNodeConfig> ComprehensionMethod::ComprehendAndCleanModel(
    ModelNodeConfig& node_config, const ContextMap& context_map) {
  // Update context using @context_map.
  bool add_braces = false;
  ContextMap new_context =
      ContextAsMap(node_config.comprehend().context(), add_braces);
  for (auto& key_value : context_map) {
    new_context[key_value.first] = key_value.second;
  }
  node_config.mutable_comprehend()->mutable_context()->CopyFrom(
      MapAsContext(new_context));

  ASSIGN_OR_RETURN(ModelNodeConfig result, ComprehendModel(node_config));
  ClearComprehension(result);
  return result;
}

ComprehensionMethod::ComprehensionMethod() {}

absl::StatusOr<std::vector<ModelNodeConfig>> ComprehensionMethod::Apply(
    ModelNodeConfig& node_config) const {
  return absl::InternalError(
      "ComprehensionMethod: Cannot use baseclass for Apply.");
}

}  // namespace wfa_virtual_people
