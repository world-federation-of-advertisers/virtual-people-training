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

#include "wfa/virtual_people/training/model_compiler/field_filter_utils.h"

#include <algorithm>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "wfa/virtual_people/common/field_filter.pb.h"

namespace wfa_virtual_people {

FieldFilterProto CreateTrueFilter() {
  FieldFilterProto filter;
  filter.set_op(FieldFilterProto::TRUE);
  return filter;
}

absl::StatusOr<std::string> GetValueOfEqualFilter(
    const FieldFilterProto& filter, absl::string_view name) {
  if (filter.op() == FieldFilterProto::EQUAL && filter.name() == name) {
    return filter.value();
  }
  if (filter.op() == FieldFilterProto::AND) {
    for (const FieldFilterProto& sub_filter : filter.sub_filters()) {
      absl::StatusOr<std::string> value =
          GetValueOfEqualFilter(sub_filter, name);
      if (value.ok()) {
        return value;
      }
    }
  }
  return absl::NotFoundError("No matching equal filter.");
}

void RemoveEqualFilter(FieldFilterProto& filter, absl::string_view name) {
  if (filter.op() == FieldFilterProto::EQUAL && filter.name() == name) {
    filter.Clear();
    filter.set_op(FieldFilterProto::TRUE);
  }
  if (filter.op() == FieldFilterProto::AND) {
    for (FieldFilterProto& sub_filter : *filter.mutable_sub_filters()) {
      RemoveEqualFilter(sub_filter, name);
    }

    // Remove any sub_filters which is TRUE filter.
    filter.mutable_sub_filters()->erase(
        std::remove_if(filter.mutable_sub_filters()->begin(),
                       filter.mutable_sub_filters()->end(),
                       [](FieldFilterProto& sub_filter) {
                         return sub_filter.op() == FieldFilterProto::TRUE;
                       }),
        filter.mutable_sub_filters()->end());

    if (filter.sub_filters_size() == 0) {
      filter.Clear();
      filter.set_op(FieldFilterProto::TRUE);
    } else if (filter.sub_filters_size() == 1) {
      FieldFilterProto filter_copy = filter.sub_filters(0);
      filter.Clear();
      filter = filter_copy;
    }
  }
}

}  // namespace wfa_virtual_people
