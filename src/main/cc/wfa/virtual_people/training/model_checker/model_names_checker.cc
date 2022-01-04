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

#include "wfa/virtual_people/training/model_checker/model_names_checker.h"

#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "wfa/virtual_people/common/model.pb.h"

namespace wfa_virtual_people {

absl::Status CheckNodeNames(const std::vector<CompiledNode>& nodes) {
  absl::flat_hash_set<std::string> names;
  for (const CompiledNode& node : nodes) {
    std::string name = node.name();
    auto [it, inserted] = names.insert(node.name());
    if (!inserted) {
      return absl::InvalidArgumentError(
          absl::StrCat("Duplicated node names: ", name));
    }
  }
  return absl::OkStatus();
}

}  // namespace wfa_virtual_people
