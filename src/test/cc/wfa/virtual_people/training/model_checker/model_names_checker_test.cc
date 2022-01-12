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

#include "absl/status/status.h"
#include "common_cpp/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/common/model.pb.h"

namespace wfa_virtual_people {
namespace {

using ::wfa::IsOk;
using ::wfa::StatusIs;

TEST(CheckNodeNamesTest, NoDuplicatedName) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
      )pb",
      &nodes.emplace_back()));
  EXPECT_THAT(CheckNodeNames(nodes), IsOk());
}

TEST(CheckNodeNamesTest, DuplicatedName) {
  std::vector<CompiledNode> nodes;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node1"
      )pb",
      &nodes.emplace_back()));
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        name: "node2"
      )pb",
      &nodes.emplace_back()));
  EXPECT_THAT(CheckNodeNames(nodes),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       "Duplicated node names: node1"));
}

}  // namespace
}  // namespace wfa_virtual_people
