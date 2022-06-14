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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common_cpp/testing/status_macros.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/training/comprehend.pb.h"

namespace wfa_virtual_people {
namespace {

using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;
using ::wfa::StatusIs;

const char kTestCsvPath[] =
    "src/test/cc/wfa/virtual_people/training/model_compiler/comprehension/"
    "test_data/test_csv.csv";

TEST(SpecUtilTest, ListSpec_NotSet) {
  ListSpec config;
  EXPECT_THAT(
      ReadListFromSpec(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set list_spec"));
}

TEST(SpecUtilTest, ListSpec_Verbatim) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        verbatim { items: "first" items: "second" }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(std::vector<std::string> items,
                       ReadListFromSpec(config));
  EXPECT_THAT(items, ElementsAre("first", "second"));
}

TEST(SpecUtilTest, ListSpec_FromCsv_InvalidFile) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { column_name: "b" }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename("");
  EXPECT_THAT(ReadListFromSpec(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "Cannot open file"));
}

TEST(SpecUtilTest, ListSpec_FromCsv_ColumnSpecNotSet) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(
      ReadListFromSpec(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "Must set column_spec"));
}

TEST(SpecUtilTest, ListSpec_FromCsv_ColumnNotFound) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { column_name: "x" }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(
      ReadListFromSpec(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "not found in header"));
}

TEST(SpecUtilTest, ListSpec_FromCsv_ColumnIndexOutOfRange) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { column_index: 4 }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(ReadListFromSpec(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "out of range"));
}

TEST(SpecUtilTest, ListSpec_FromCsv) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { column_name: "b" }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  ASSERT_OK_AND_ASSIGN(std::vector<std::string> items,
                       ReadListFromSpec(config));
  EXPECT_THAT(items, ElementsAre("X", "Y", "Z", "W"));
}

TEST(SpecUtilTest, ListSpec_FromCsv_NotSorted) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { column_name: "d" }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  ASSERT_OK_AND_ASSIGN(std::vector<std::string> items,
                       ReadListFromSpec(config));
  EXPECT_THAT(items, ElementsAre("9", "6", "3", "20"));
}

TEST(SpecUtilTest, ListSpec_FromCsv_Sorted) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { column_name: "d" make_it_sorted_set: true }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  ASSERT_OK_AND_ASSIGN(std::vector<std::string> items,
                       ReadListFromSpec(config));
  // Sorted as string.
  EXPECT_THAT(items, ElementsAre("20", "3", "6", "9"));
}

TEST(SpecUtilTest, ListSpec_FromCsv_HasDuplicate) {
  ListSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { column_name: "a" }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(ReadListFromSpec(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "must be unique"));
}

TEST(SpecUtilTest, MapSpec_NotSet) {
  MapSpec config;
  EXPECT_THAT(
      ReadMapFromSpec(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set map_spec"));
}

TEST(SpecUtilTest, MapSpec_Verbatim) {
  MapSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        verbatim {
          items { key: "us" values: "xa" values: "xb" }
          items { key: "ca" values: "yb" values: "ya" }
        }
      )pb",
      &config));
  ASSERT_OK_AND_ASSIGN(StringToStringsMap mapping, ReadMapFromSpec(config));
  EXPECT_THAT(mapping,
              UnorderedElementsAre(Pair("us", ElementsAre("xa", "xb")),
                                   Pair("ca", ElementsAre("yb", "ya"))));
}

TEST(SpecUtilTest, MapSpec_FromCsv_InvalidFile) {
  MapSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv {}
      )pb",
      &config));
  config.mutable_from_csv()->set_filename("");
  EXPECT_THAT(ReadMapFromSpec(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument, "Cannot open file"));
}

TEST(SpecUtilTest, MapSpec_FromCsv_KeyColumnNotSet) {
  MapSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv {}
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(
      ReadMapFromSpec(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "must set key_column_name"));
}

TEST(SpecUtilTest, MapSpec_FromCsv_ValueColumnsNotSet) {
  MapSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv { key_column_name: "a" }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(ReadMapFromSpec(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "must set value_column_names"));
}

TEST(SpecUtilTest, MapSpec_FromCsv_ValueColumnsEmpty) {
  MapSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv {
          key_column_name: "a"
          value_column_names {}
        }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(ReadMapFromSpec(config).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "at least 1 item in value_column_names"));
}

TEST(SpecUtilTest, MapSpec_FromCsv_ColumnNotFound) {
  MapSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv {
          key_column_name: "a"
          value_column_names { items: "x" }
        }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  EXPECT_THAT(
      ReadMapFromSpec(config).status(),
      StatusIs(absl::StatusCode::kInvalidArgument, "not found in header"));
}

TEST(SpecUtilTest, MapSpec_FromCsv) {
  MapSpec config;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        from_csv {
          key_column_name: "b"
          value_column_names { items: "d" items: "a" items: "c" }
        }
      )pb",
      &config));
  config.mutable_from_csv()->set_filename(kTestCsvPath);
  ASSERT_OK_AND_ASSIGN(StringToStringsMap mapping, ReadMapFromSpec(config));
  EXPECT_THAT(mapping,
              UnorderedElementsAre(Pair("W", ElementsAre("20", "1", "10")),
                                   Pair("X", ElementsAre("9", "1", "2")),
                                   Pair("Y", ElementsAre("6", "4", "5")),
                                   Pair("Z", ElementsAre("3", "7", "8"))));
}

}  // namespace
}  // namespace wfa_virtual_people
