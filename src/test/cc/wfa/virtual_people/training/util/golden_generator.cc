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

// This is a tool to generate golden files for binaries based on input config.
// Example usage:
// bazel build -c opt \
// //src/test/cc/wfa/virtual_people/training/util:golden_generator.cc
// bazel-bin/src/test/cc/wfa/virtual_people/training/util/golden_generator
// or
// bazel build -c opt \
// //src/test/cc/wfa/virtual_people/training/util:golden_generator.cc
// bazel-bin/src/test/cc/wfa/virtual_people/training/util/golden_generator
// --input_path=path/to/config.textproto

#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "common_cpp/protobuf_util/textproto_io.h"
#include "glog/logging.h"
#include "wfa/virtual_people/common/config.pb.h"
#include "wfa/virtual_people/common/integration_testing_framework/golden_generator.h"

ABSL_FLAG(
    std::string, input_path,
    "src/test/cc/wfa/virtual_people/training/util/test_data/config.textproto",
    "Path to the input config textproto");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  std::string input_path = absl::GetFlag(FLAGS_input_path);
  CHECK(!input_path.empty()) << "input_path is not set.";

  wfa_virtual_people::IntegrationTestList config;
  absl::Status readConfigStatus = wfa::ReadTextProtoFile(input_path, config);
  CHECK(readConfigStatus == absl::OkStatus())
      << "Read Config Status: " << readConfigStatus;

  std::vector<std::string> executeVector(GoldenGenerator(config));

  for (std::string execute : executeVector) {
    CHECK(std::system(execute.c_str()))
        << "Execution of " << execute << " failed.";
  }

  return 0;
}