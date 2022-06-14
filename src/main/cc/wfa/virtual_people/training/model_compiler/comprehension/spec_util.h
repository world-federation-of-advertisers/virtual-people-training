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

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_SPEC_UTIL_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_SPEC_UTIL_H_

#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "wfa/virtual_people/training/comprehend.pb.h"

namespace wfa_virtual_people {

// Helpers for loading ListSpec and MapSpec.
// ListSpec proto is specification for a list of strings.
// MapSpec proto is specification for a string-to-strings map.

// Map from a string to a list of strings.
typedef absl::flat_hash_map<std::string, std::vector<std::string>>
    StringToStringsMap;

absl::StatusOr<std::vector<std::string>> ReadListFromSpec(const ListSpec& spec);

absl::StatusOr<StringToStringsMap> ReadMapFromSpec(const MapSpec& spec);

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_COMPREHENSION_SPEC_UTIL_H_
