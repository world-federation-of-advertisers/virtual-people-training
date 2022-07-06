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

#include <fcntl.h>

#include <cstdlib>
#include <string>

#include "glog/logging.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "wfa/virtual_people/training/model_config.pb.h"

using namespace ::google::protobuf;
using namespace ::google::protobuf::io;
using namespace ::google::protobuf::util;

namespace wfa_virtual_people {
namespace {

void ReadTextProtoFile(std::string path, Message& message) {
  CHECK(!path.empty()) << "No path set";
  int fd = open(path.c_str(), O_RDONLY);
  CHECK(fd > 0) << "Unable to open file: " << path;
  FileInputStream fstream(fd);
  CHECK(TextFormat::Parse(&fstream, &message))
      << "Unable to parse textproto file: " << path;
}

bool ProtoEquals(std::string expectedPath, std::string outputPath) {
  CompiledNode expected;
  ReadTextProtoFile(expectedPath, expected);

  CompiledNode output;
  ReadTextProtoFile(outputPath, output);

  MessageDifferencer diff;
  return diff.Equals(expected, output);
}

TEST(ModelCompilerMainTest, PopulationNode) {
  std::string expectedPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "compiled_node_for_population_node.textproto";
  std::string outputPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/"
      "compiled_node_for_population_node.textproto";

  EXPECT_TRUE(ProtoEquals(expectedPath, outputPath));
}

TEST(ModelCompilerMainTest,
     PopulationNodeRedistributeProbabilitiesForEmptyPools) {
  std::string expectedPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "compiled_node_for_population_node_redistribute_probabilities_for_empty_"
      "pools.textproto";
  std::string outputPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/"
      "compiled_node_for_population_node_redistribute_probabilities_for_empty_"
      "pools.textproto";

  EXPECT_TRUE(ProtoEquals(expectedPath, outputPath));
}

TEST(ModelCompilerMainTest, PopulationNodeKappaLessThanOne) {
  std::string expectedPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "compiled_node_for_population_node_kappa_less_than_one.textproto";
  std::string outputPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/"
      "compiled_node_for_population_node_kappa_less_than_one.textproto";

  EXPECT_TRUE(ProtoEquals(expectedPath, outputPath));
}

TEST(ModelCompilerMainTest, PopulationNodeDiscretization) {
  std::string expectedPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/test_data/"
      "compiled_node_for_population_node_discretization.textproto";
  std::string outputPath =
      "src/test/cc/wfa/virtual_people/training/model_compiler/"
      "compiled_node_for_population_node_discretization.textproto";

  EXPECT_TRUE(ProtoEquals(expectedPath, outputPath));
}

}  // namespace
}  // namespace wfa_virtual_people