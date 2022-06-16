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

// This is a tool to compile a ModelNodeConfig to a model represented by its
// root node in CompiledNode.
// The input ModelNodeConfig is required to be in textproto.
// The output CompiledNode is formatted in textproto.
// Example usage:
// bazel build -c opt \
// //src/main/cc/wfa/virtual_people/training/model_compiler:compiler_main
// bazel-bin/src/main/cc/wfa/virtual_people/training/model_compiler/\
// compiler_main \
// --input_path=/tmp/model_compiler/model_config.textproto \
// --output_path=/tmp/model_compiler/model.textproto

#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common_cpp/protobuf_util/textproto_io.h"
#include "glog/logging.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_compiler/compiler.h"
#include "wfa/virtual_people/training/model_compiler/comprehension/comprehension_method.h"
#include "wfa/virtual_people/training/model_compiler/comprehension/contextual_boolean_expression.h"
#include "wfa/virtual_people/training/model_config.pb.h"

ABSL_FLAG(std::string, input_path, "",
          "Path to the input ModelNodeConfig textproto.");
ABSL_FLAG(std::string, output_path, "",
          "Path to the output CompiledNode textproto.");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  google::InitGoogleLogging(argv[0]);

  std::string input_path = absl::GetFlag(FLAGS_input_path);
  CHECK(!input_path.empty()) << "input_path is not set.";

  std::string output_path = absl::GetFlag(FLAGS_output_path);
  CHECK(!output_path.empty()) << "output_path is not set.";

  wfa_virtual_people::ModelNodeConfig config;
  absl::Status read_status = wfa::ReadTextProtoFile(input_path, config);
  CHECK(read_status.ok()) << read_status;

  wfa_virtual_people::ContextMap context_map;
  absl::StatusOr<wfa_virtual_people::ModelNodeConfig> comprehended =
      wfa_virtual_people::ComprehensionMethod::ComprehendAndCleanModel(
          config, context_map);
  CHECK(comprehended.status().ok()) << comprehended.status();
  config = *comprehended;

  absl::StatusOr<wfa_virtual_people::CompiledNode> model =
      wfa_virtual_people::CompileModel(config);
  CHECK(model.ok()) << model.status();

  absl::Status write_status = wfa::WriteTextProtoFile(output_path, *model);
  CHECK(write_status.ok()) << write_status;

  return 0;
}
