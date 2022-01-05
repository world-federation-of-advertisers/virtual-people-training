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

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_CHECKER_MODEL_NAMES_CHECKER_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_CHECKER_MODEL_NAMES_CHECKER_H_

#include <vector>

#include "absl/status/status.h"
#include "wfa/virtual_people/common/model.pb.h"

namespace wfa_virtual_people {

// Return error status if there are any duplicated names in @nodes.
//
// We assume that any child node should be referenced by index rather than the
// CompiledNode object in @nodes.
absl::Status CheckNodeNames(const std::vector<CompiledNode>& nodes);

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_CHECKER_MODEL_NAMES_CHECKER_H_
