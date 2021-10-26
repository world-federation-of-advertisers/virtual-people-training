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

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_FIELD_FILTER_UTILS_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_FIELD_FILTER_UTILS_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "wfa/virtual_people/common/field_filter.pb.h"

namespace wfa_virtual_people {

// Create a FieldFilterProto with op as TRUE.
FieldFilterProto CreateTrueFilter();

// If @filter.op is EQUAL, return the value of @filter.value if the value of
// @filter.name matches @name.
// If @filter.op is AND, apply the check to each of the @filter.sub_filters, and
// return the value if any matching entity is found.
// Return error status if no matching field filter is found.
// Example 1:
// If filter is
//   op: EQUAL
//   name: "a"
//   value: "1"
// And name is "a"
// Return "1"
//
// Example 2:
// If filter is
//   op: AND
//   sub_filters {
//     op: EQUAL
//     name: "a"
//     value: "1"
//   }
//   sub_filters {
//     op: EQUAL
//     name: "b"
//     value: "2"
//   }
// And name is "a"
// Return "1"
//
// Example 3:
// If filter is
//   op: AND
//   sub_filters {
//     op: EQUAL
//     name: "b"
//     value: "1"
//   }
//   sub_filters {
//     op: EQUAL
//     name: "c"
//     value: "2"
//   }
// And name is "a"
// Return error status.
absl::StatusOr<std::string> GetValueOfEqualFilter(
    const FieldFilterProto& filter, absl::string_view name);

// If @filter.op is EQUAL, change @filter to be a TRUE field filter if the value
// of @filter.name matches @name.
// If @filter.op is AND,
// * Appy the check in each of @filter.sub_filters.
// * Remove any sub_filters that is TRUE filter.
// * If only 1 sub_filters is left, flatten the @filter.
// * If all the sub_filters are removed, add a TRUE filter in @filter.
//
// Example 1:
// If filter is
//   op: EQUAL
//   name: "a"
//   value: "1"
// And name is "a"
// The filter will be
//   op: TRUE
//
// Example 2:
// If filter is
//   op: AND
//   sub_filters {
//     op: EQUAL
//     name: "a"
//     value: "1"
//   }
//   sub_filters {
//     op: EQUAL
//     name: "b"
//     value: "2"
//   }
// And name is "a"
// The filter will be
//   op: EQUAL
//   name: "b"
//   value: "2"
//
// Example 3:
// If filter is
//   op: AND
//   sub_filters {
//     op: EQUAL
//     name: "a"
//     value: "1"
//   }
//   sub_filters {
//     op: EQUAL
//     name: "a"
//     value: "2"
//   }
// And name is "a"
// The filter will be
//   op: TRUE
void RemoveEqualFilter(FieldFilterProto& filter, absl::string_view name);

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_FIELD_FILTER_UTILS_H_
