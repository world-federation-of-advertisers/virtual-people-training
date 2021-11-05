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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common_cpp/testing/common_matchers.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/common/field_filter.pb.h"

namespace wfa_virtual_people {
namespace {

using ::wfa::EqualsProto;
using ::wfa::IsOkAndHolds;
using ::wfa::StatusIs;

TEST(GetValueOfEqualFilterTest, MatchingFilter) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: EQUAL name: "person_country_code" value: "COUNTRY_CODE_1"
      )pb",
      &filter));
  EXPECT_THAT(GetValueOfEqualFilter(filter, "person_country_code"),
              IsOkAndHolds("COUNTRY_CODE_1"));
}

TEST(GetValueOfEqualFilterTest, MatchingSubFilter) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: AND
        sub_filters { op: TRUE }
        sub_filters {
          op: EQUAL
          name: "person_country_code"
          value: "COUNTRY_CODE_1"
        }
      )pb",
      &filter));
  EXPECT_THAT(GetValueOfEqualFilter(filter, "person_country_code"),
              IsOkAndHolds("COUNTRY_CODE_1"));
}

TEST(GetValueOfEqualFilterTest, NoMatchingFilter) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: TRUE
      )pb",
      &filter));
  EXPECT_THAT(
      GetValueOfEqualFilter(filter, "person_country_code").status(),
      StatusIs(absl::StatusCode::kNotFound, "No matching equal filter."));
}

TEST(RemoveEqualFilterTest, NoMatchingFilter) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: EQUAL name: "person_country_code" value: "COUNTRY_CODE_1"
      )pb",
      &filter));
  RemoveEqualFilter(filter, "person_region_code");

  FieldFilterProto expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: EQUAL name: "person_country_code" value: "COUNTRY_CODE_1"
      )pb",
      &expected));
  EXPECT_THAT(filter, EqualsProto(expected));
}

TEST(RemoveEqualFilterTest, HasMatchingFilter) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: EQUAL name: "person_country_code" value: "COUNTRY_CODE_1"
      )pb",
      &filter));
  RemoveEqualFilter(filter, "person_country_code");

  FieldFilterProto expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: TRUE
      )pb",
      &expected));
  EXPECT_THAT(filter, EqualsProto(expected));
}

TEST(RemoveEqualFilterTest, HasMatchingSubFilter) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: AND
        sub_filters {
          op: EQUAL
          name: "person_country_code"
          value: "COUNTRY_CODE_1"
        }
        sub_filters {
          op: EQUAL
          name: "person_region_code"
          value: "REGION_CODE_1"
        }
        sub_filters {
          op: EQUAL
          name: "person_region_code"
          value: "REGION_CODE_2"
        }
      )pb",
      &filter));
  RemoveEqualFilter(filter, "person_country_code");

  FieldFilterProto expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: AND
        sub_filters {
          op: EQUAL
          name: "person_region_code"
          value: "REGION_CODE_1"
        }
        sub_filters {
          op: EQUAL
          name: "person_region_code"
          value: "REGION_CODE_2"
        }
      )pb",
      &expected));
  EXPECT_THAT(filter, EqualsProto(expected));
}

TEST(RemoveEqualFilterTest, HasMatchingSubFilterFlattened) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: AND
        sub_filters {
          op: EQUAL
          name: "person_country_code"
          value: "COUNTRY_CODE_1"
        }
        sub_filters {
          op: EQUAL
          name: "person_region_code"
          value: "REGION_CODE_1"
        }
      )pb",
      &filter));
  RemoveEqualFilter(filter, "person_country_code");

  FieldFilterProto expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: EQUAL name: "person_region_code" value: "REGION_CODE_1"
      )pb",
      &expected));
  EXPECT_THAT(filter, EqualsProto(expected));
}

TEST(RemoveEqualFilterTest, HasMatchingSubFilterAllRemoved) {
  FieldFilterProto filter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: AND
        sub_filters {
          op: EQUAL
          name: "person_country_code"
          value: "COUNTRY_CODE_1"
        }
      )pb",
      &filter));
  RemoveEqualFilter(filter, "person_country_code");

  FieldFilterProto expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        op: TRUE
      )pb",
      &expected));
  EXPECT_THAT(filter, EqualsProto(expected));
}

}  // namespace
}  // namespace wfa_virtual_people
