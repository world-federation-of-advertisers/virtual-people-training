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

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_SPECIFICATION_UTILS_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_SPECIFICATION_UTILS_H_

#include "absl/status/statusor.h"
#include "wfa/virtual_people/common/field_filter.pb.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_config.pb.h"

namespace wfa_virtual_people {

// Create a FieldFilterProto from FieldFilterProtoSpecification.
absl::StatusOr<FieldFilterProto> CompileFieldFilterProto(
    const FieldFilterProtoSpecification& config);

// Create an AttributesUpdaters from AttributesUpdatersSpecification.
absl::StatusOr<BranchNode::AttributesUpdaters> CompileAttributesUpdaters(
    const ModelNodeConfig::AttributesUpdatersSpecification& config);

// Create a Multiplicity from MultiplicitySpecification.
absl::StatusOr<Multiplicity> CompileMultiplicity(
    const MultiplicitySpecification& config);

// Create a ActivityDensityFunction from ActivityDensityFunctionSpecification.
absl::StatusOr<ActivityDensityFunction> CompileActivityDensityFunction(
    const ActivityDensityFunctionSpecification& config);

// Create a Multipool from MultipoolSpecification.
absl::StatusOr<Multipool> CompileMultipool(
    const MultipoolSpecification& config);

// Create a CensusRecords from CensusRecordsSpecification.
absl::StatusOr<CensusRecords> CompileCensusRecords(
    const CensusRecordsSpecification& config);

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_SPECIFICATION_UTILS_H_
