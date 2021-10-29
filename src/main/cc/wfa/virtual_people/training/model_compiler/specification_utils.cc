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

#include <fcntl.h>

#include <filesystem>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "common_cpp/macros/macros.h"
#include "common_cpp/protobuf_util/textproto_io.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "wfa/virtual_people/common/field_filter.pb.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_compiler/compiler.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {

namespace {

using ::wfa::ReadTextProtoFile;
using AttributesUpdater = BranchNode::AttributesUpdater;
using AttributesUpdaters = BranchNode::AttributesUpdaters;
using AttributesUpdaterSpecification =
    ModelNodeConfig::AttributesUpdaterSpecification;
using AttributesUpdatersSpecification =
    ModelNodeConfig::AttributesUpdatersSpecification;

template <typename ProtoType, typename ProtoSpecificationType>
absl::StatusOr<ProtoType> CompileFromSpecification(
    const ProtoSpecificationType& config) {
  if (config.has_verbatim()) {
    return config.verbatim();
  }
  if (config.has_from_file()) {
    ProtoType proto_output;
    RETURN_IF_ERROR(ReadTextProtoFile(config.from_file(), proto_output));
    return proto_output;
  }
  return absl::InvalidArgumentError(absl::StrCat(
      "Neither verbatim nor from_file is set: ", config.DebugString()));
}

absl::StatusOr<UpdateMatrix> CompileUpdateMatrix(
    const UpdateMatrixSpecification& config) {
  return CompileFromSpecification<UpdateMatrix, UpdateMatrixSpecification>(
      config);
}

absl::StatusOr<SparseUpdateMatrix> CompileSparseUpdateMatrix(
    const SparseUpdateMatrixSpecification& config) {
  return CompileFromSpecification<SparseUpdateMatrix,
                                  SparseUpdateMatrixSpecification>(config);
}

absl::StatusOr<ConditionalMerge> CompileConditionalMerge(
    const ConditionalMergeSpecification& config) {
  return CompileFromSpecification<ConditionalMerge,
                                  ConditionalMergeSpecification>(config);
}

absl::StatusOr<ConditionalAssignment> CompileConditionalAssignment(
    const ConditionalAssignmentSpecification& config) {
  return CompileFromSpecification<ConditionalAssignment,
                                  ConditionalAssignmentSpecification>(config);
}

absl::StatusOr<CompiledNode> CompileCompiledNode(
    const CompiledNodeSpecification& config) {
  switch (config.source_case()) {
    case CompiledNodeSpecification::kVerbatim:
      return config.verbatim();
    case CompiledNodeSpecification::kCompiledNodeFromFile: {
      CompiledNode node;
      RETURN_IF_ERROR(
          ReadTextProtoFile(config.compiled_node_from_file(), node));
      return node;
    }
    case CompiledNodeSpecification::kModelNodeConfig:
      return CompileModel(config.model_node_config());
    case CompiledNodeSpecification::kModelNodeConfigFromFile: {
      ModelNodeConfig node_config;
      RETURN_IF_ERROR(
          ReadTextProtoFile(config.model_node_config_from_file(), node_config));
      return CompileModel(node_config);
    }
    default:
      return absl::InvalidArgumentError(absl::StrCat(
          "None of verbatim, compiled_node_from_file, model_node_config, or "
          "model_node_config_from_file is set: ",
          config.DebugString()));
  }
}

absl::StatusOr<UpdateTree> CompileUpdateTree(
    const UpdateTreeSpecification& config) {
  if (config.has_root_node()) {
    UpdateTree update_tree;
    ASSIGN_OR_RETURN(*update_tree.mutable_root(),
                     CompileCompiledNode(config.root_node()));
    return update_tree;
  }
  return absl::InvalidArgumentError(
      absl::StrCat("root_node is not set: ", config.DebugString()));
}

absl::StatusOr<AttributesUpdater> CompileAttributesUpdater(
    const AttributesUpdaterSpecification& config) {
  AttributesUpdater updater;
  switch (config.update_case()) {
    case AttributesUpdaterSpecification::kUpdateMatrix: {
      ASSIGN_OR_RETURN(*updater.mutable_update_matrix(),
                       CompileUpdateMatrix(config.update_matrix()));
      break;
    }
    case AttributesUpdaterSpecification::kSparseUpdateMatrix: {
      ASSIGN_OR_RETURN(
          *updater.mutable_sparse_update_matrix(),
          CompileSparseUpdateMatrix(config.sparse_update_matrix()));
      break;
    }
    case AttributesUpdaterSpecification::kConditionalMerge: {
      ASSIGN_OR_RETURN(*updater.mutable_conditional_merge(),
                       CompileConditionalMerge(config.conditional_merge()));
      break;
    }
    case AttributesUpdaterSpecification::kUpdateTree: {
      ASSIGN_OR_RETURN(*updater.mutable_update_tree(),
                       CompileUpdateTree(config.update_tree()));
      break;
    }
    case AttributesUpdaterSpecification::kConditionalAssignment: {
      ASSIGN_OR_RETURN(
          *updater.mutable_conditional_assignment(),
          CompileConditionalAssignment(config.conditional_assignment()));
      break;
    }
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("update is not set: ", config.DebugString()));
  }
  return updater;
}

}  // namespace

absl::StatusOr<FieldFilterProto> CompileFieldFilterProto(
    const FieldFilterProtoSpecification& config) {
  return CompileFromSpecification<FieldFilterProto,
                                  FieldFilterProtoSpecification>(config);
}

absl::StatusOr<AttributesUpdaters> CompileAttributesUpdaters(
    const AttributesUpdatersSpecification& config) {
  AttributesUpdaters updaters;
  for (const AttributesUpdaterSpecification& update_config : config.updates()) {
    ASSIGN_OR_RETURN(*updaters.add_updates(),
                     CompileAttributesUpdater(update_config));
  }
  return updaters;
}

absl::StatusOr<Multiplicity> CompileMultiplicity(
    const MultiplicitySpecification& config) {
  return CompileFromSpecification<Multiplicity, MultiplicitySpecification>(
      config);
}

absl::StatusOr<ActivityDensityFunction> CompileActivityDensityFunction(
    const ActivityDensityFunctionSpecification& config) {
  return CompileFromSpecification<ActivityDensityFunction,
                                  ActivityDensityFunctionSpecification>(config);
}

absl::StatusOr<Multipool> CompileMultipool(
    const MultipoolSpecification& config) {
  return CompileFromSpecification<Multipool, MultipoolSpecification>(config);
}

absl::StatusOr<CensusRecords> CompileCensusRecords(
    const CensusRecordsSpecification& config) {
  return CompileFromSpecification<CensusRecords, CensusRecordsSpecification>(
      config);
}

}  // namespace wfa_virtual_people
