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

// This is a tool to do sanity check to a model, which is composed of a list of
// CompiledNodes, and each child node is referenced by index.
// The input model_path is required to be a Riegeli file in CompiledNode
// protobuf.
// Example usage:
// bazel build -c opt \
// //src/main/cc/wfa/virtual_people/training/model_checher:model_checker_main
// bazel-bin/src/main/cc/wfa/virtual_people/training/model_checher/\
// model_checker_main \
// --model_path=/tmp/model_checher/model.riegeli

#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "common_cpp/protobuf_util/riegeli_io.h"
#include "glog/logging.h"
#include "wfa/virtual_people/common/model.pb.h"
#include "wfa/virtual_people/training/model_checker/model_names_checker.h"
#include "wfa/virtual_people/training/model_checker/model_seeds_checker.h"

ABSL_FLAG(std::string, model_path, "",
          "Path to the input CompiledNode Riegeli file.");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  google::InitGoogleLogging(argv[0]);

  // Read the model from the given Riegeli file.
  std::string model_path = absl::GetFlag(FLAGS_model_path);
  CHECK(!model_path.empty()) << "model_path is not set.";
  std::vector<wfa_virtual_people::CompiledNode> nodes;
  absl::Status read_status =
      wfa::ReadRiegeliFile<wfa_virtual_people::CompiledNode>(model_path, nodes);
  CHECK(read_status.ok()) << read_status;

  // TODO(@tcsnfkx): Validate the indexes of the nodes.

  absl::Status names_status = wfa_virtual_people::CheckNodeNames(nodes);
  CHECK(names_status.ok()) << names_status;

  absl::Status seeds_status = wfa_virtual_people::CheckNodeSeeds(nodes);
  CHECK(seeds_status.ok()) << seeds_status;

  return 0;
}
