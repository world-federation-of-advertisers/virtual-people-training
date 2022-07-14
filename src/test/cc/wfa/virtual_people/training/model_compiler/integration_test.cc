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

#include <fcntl.h>

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "common_cpp/protobuf_util/textproto_io.h"
#include "glog/logging.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "tools/cpp/runfiles/runfiles.h"
#include "wfa/virtual_people/training/config.pb.h"
#include "wfa/virtual_people/training/model_config.pb.h"  // temp for testing

using bazel::tools::cpp::runfiles::Runfiles;
using ::wfa::ReadTextProtoFile;

namespace wfa_virtual_people {
namespace {

struct Targets {
  std::string name;
  std::string output;
  std::string golden;
};

// Returns a list of Targets parsed from the input config.
// Additionally, locates the runfiles path for the given binary and executes the
// binary with it's given binary_parameters from the input config. Depending on
// if the golden file exists or not, will generate a golden file or append
// to targets.
std::vector<Targets> ParseConfig(std::string path) {
  IntegrationTestList config;

  absl::Status readConfigStatus = ReadTextProtoFile(path, config);

  std::vector<Targets> targets;
  std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest());
  std::string name, output, golden, rPath, execute;

  for (int testIndex = 0; testIndex < config.tests_size(); testIndex++) {
    IntegrationTest it = config.tests().at(testIndex);
    rPath = runfiles->Rlocation(it.binary());
    for (int testCaseIndex = 0; testCaseIndex < it.test_cases_size();
         testCaseIndex++) {
      TestCase tc = it.test_cases().at(testCaseIndex);
      name = it.name() + "_" + tc.name();
      execute = rPath;
      for (int binaryParameterIndex = 0;
           binaryParameterIndex < tc.binary_parameters_size();
           binaryParameterIndex++) {
        BinaryParameter bp = tc.binary_parameters().at(binaryParameterIndex);
        execute += " --" + bp.name() + "=" + bp.value();
        if (!bp.golden().empty() && open(bp.golden().data(), O_RDONLY) != -1) {
          output = bp.value();
          golden = bp.golden();
          targets.push_back({name, output, golden});
        } else if (!bp.golden().empty()) {
          execute.erase(execute.find(bp.value()));
          execute += bp.golden();
        }
        /*
         * TODO(wastadtlander): For this section ^^^ need to make it so when
         * executed via system it actually generates the file. The issue is that
         * because the test is running via Bazel, Bazel doesn't let you write to
         * your workspace.
         */
      }
      std::system(execute.c_str());
    }
  }

  return targets;
}

class IntegrationTestParamaterizedFixture
    : public ::testing::TestWithParam<Targets> {
 protected:
  google::protobuf::util::MessageDifferencer diff;
};

TEST_P(IntegrationTestParamaterizedFixture, Test) {
  Targets targets(GetParam());

  CompiledNode output;
  absl::Status outputStatus = ReadTextProtoFile(targets.output, output);

  CompiledNode golden;
  absl::Status goldenStatus = ReadTextProtoFile(targets.golden, golden);

  ASSERT_TRUE(diff.Equals(output, golden));
}

INSTANTIATE_TEST_SUITE_P(
    IntegrationTest, IntegrationTestParamaterizedFixture,
    ::testing::ValuesIn(
        ParseConfig("src/test/cc/wfa/virtual_people/training/model_compiler/"
                    "test_data/config.textproto")),
    [](const ::testing::TestParamInfo<
        IntegrationTestParamaterizedFixture::ParamType>& info) {
      std::string name = info.param.name;
      return name;
    });

}  // namespace
}  // namespace wfa_virtual_people
