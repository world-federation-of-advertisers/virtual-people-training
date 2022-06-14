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

#include "wfa/virtual_people/training/model_compiler/comprehension/spec_util.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "common_cpp/macros/macros.h"
#include "riegeli/bytes/string_reader.h"
#include "riegeli/csv/csv_reader.h"
#include "riegeli/csv/csv_record.h"
#include "wfa/virtual_people/training/comprehend.pb.h"

namespace wfa_virtual_people {

namespace {

absl::StatusOr<std::string> GetFileContent(const std::string& filename) {
  std::ifstream input(filename);
  if (!input.is_open()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Cannot open file ", filename));
  }
  std::ostringstream output;
  output << input.rdbuf();
  return output.str();
}

// Get index of @column_name in @header.
absl::StatusOr<int> GetColumnIndex(absl::string_view column_name,
                                   const std::vector<std::string>& header) {
  auto iter = std::find(header.begin(), header.end(), column_name);
  if (iter == header.end()) {
    return absl::InvalidArgumentError(
        absl::StrCat("column ", column_name, " not found in header."));
  }
  return iter - header.begin();
}

// Get indices of @column_names in @header.
absl::StatusOr<std::vector<int>> GetColumnsIndex(
    const StringList& column_names, const std::vector<std::string>& header) {
  std::vector<int> result;
  for (absl::string_view column : column_names.items()) {
    result.emplace_back();
    ASSIGN_OR_RETURN(result.back(), GetColumnIndex(column, header));
  }
  return result;
}

// Get index in @header for column in @csv_spec.
absl::StatusOr<int> GetColumnIndex(const ListSpec::ListFromCSV& csv_spec,
                                   const std::vector<std::string>& header) {
  if (csv_spec.has_column_name()) {
    return GetColumnIndex(csv_spec.column_name(), header);
  } else if (csv_spec.has_column_index()) {
    if (csv_spec.column_index() >= header.size()) {
      return absl::InvalidArgumentError(
          absl::StrCat("column index ", csv_spec.column_index(),
                       " out of range: ", csv_spec.filename()));
    }
    return csv_spec.column_index();
  } else {
    return absl::InvalidArgumentError(
        absl::StrCat("Must set column_spec.", csv_spec.DebugString()));
  }
}

// Read string list from file as specified in @csv_spec.
absl::StatusOr<std::vector<std::string>> ReadListFromCsv(
    const ListSpec::ListFromCSV& csv_spec) {
  ASSIGN_OR_RETURN(std::string content, GetFileContent(csv_spec.filename()));
  riegeli::CsvReader<riegeli::StringReader<std::string>> csv_reader{
      riegeli::StringReader<std::string>(content),
      riegeli::CsvReaderBase::Options().set_comment('#')};

  std::vector<std::string> header;
  if (!csv_reader.ReadRecord(header)) {
    return absl::InvalidArgumentError(
        absl::StrCat("Failed to read header: ", csv_spec.filename()));
  }
  ASSIGN_OR_RETURN(int column_index, GetColumnIndex(csv_spec, header));

  std::vector<std::string> record;
  std::vector<std::string> result;
  while (csv_reader.ReadRecord(record)) {
    if (record.size() <= column_index) {
      return absl::InvalidArgumentError(
          absl::StrCat("column index ", column_index,
                       " out of range: ", csv_spec.filename()));
    }
    result.emplace_back(record[column_index]);
  }

  // All values must be unique.
  if (csv_spec.has_make_it_sorted_set() && csv_spec.make_it_sorted_set()) {
    std::sort(result.begin(), result.end());
  }
  absl::flat_hash_set<std::string> result_set;
  for (std::string x : result) {
    if (!result_set.insert(x).second) {
      return absl::InvalidArgumentError(
          absl::StrCat("All values in for_each must be unique. Consider "
                       "setting make_it_sorted_set to true.",
                       csv_spec.filename()));
    }
  }

  return result;
}

// Read string-to-strings-map from file as specified in @csv_spec.
absl::StatusOr<StringToStringsMap> ReadMapFromCsv(
    const MapSpec::TableFromCSV& csv_spec) {
  ASSIGN_OR_RETURN(std::string content, GetFileContent(csv_spec.filename()));
  riegeli::CsvReader<riegeli::StringReader<std::string>> csv_reader{
      riegeli::StringReader<std::string>(content),
      riegeli::CsvReaderBase::Options().set_comment('#')};

  std::vector<std::string> header;
  if (!csv_reader.ReadRecord(header)) {
    return absl::InvalidArgumentError(
        absl::StrCat("Failed to read header: ", csv_spec.filename()));
  }

  if (!csv_spec.has_key_column_name()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "MapSpec must set key_column_name: ", csv_spec.DebugString()));
  }

  if (!csv_spec.has_value_column_names()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "MapSpec must set value_column_names: ", csv_spec.DebugString()));
  }

  if (csv_spec.value_column_names().items().size() == 0) {
    return absl::InvalidArgumentError(absl::StrCat(
        "MapSpec must have at least 1 item in value_column_names: ",
        csv_spec.DebugString()));
  }

  ASSIGN_OR_RETURN(int key_column_index,
                   GetColumnIndex(csv_spec.key_column_name(), header));
  ASSIGN_OR_RETURN(std::vector<int> value_column_index,
                   GetColumnsIndex(csv_spec.value_column_names(), header));

  int max_index_used =
      *max_element(value_column_index.begin(), value_column_index.end());
  if (key_column_index > max_index_used) {
    max_index_used = key_column_index;
  }

  std::vector<std::string> record;
  StringToStringsMap result;
  while (csv_reader.ReadRecord(record)) {
    if (record.size() <= max_index_used) {
      return absl::InvalidArgumentError(
          absl::StrCat("column index ", max_index_used,
                       " out of range: ", csv_spec.filename()));
    }
    std::vector<std::string> values;
    for (int i : value_column_index) {
      values.emplace_back(record[i]);
    }
    result[record[key_column_index]] = std::move(values);
  }

  return result;
}

}  // namespace

absl::StatusOr<std::vector<std::string>> ReadListFromSpec(
    const ListSpec& spec) {
  if (spec.has_verbatim()) {
    auto items = spec.verbatim().items();
    return std::vector<std::string>(items.begin(), items.end());
  }

  if (spec.has_from_csv()) {
    return ReadListFromCsv(spec.from_csv());
  }

  return absl::InvalidArgumentError(
      absl::StrCat("ListSpec must set list_spec.", spec.DebugString()));
}

absl::StatusOr<StringToStringsMap> ReadMapFromSpec(const MapSpec& spec) {
  if (spec.has_verbatim()) {
    StringToStringsMap result;
    for (auto& item : spec.verbatim().items()) {
      auto& values = item.values();
      result[item.key()] =
          std::vector<std::string>(values.begin(), values.end());
    }
    return result;
  }

  if (spec.has_from_csv()) {
    return ReadMapFromCsv(spec.from_csv());
  }

  return absl::InvalidArgumentError(
      absl::StrCat("MapSpec must set map_spec.", spec.DebugString()));
}

}  // namespace wfa_virtual_people
