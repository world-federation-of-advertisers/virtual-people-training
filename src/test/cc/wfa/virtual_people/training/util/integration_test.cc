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
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "tools/cpp/runfiles/runfiles.h"
#include "wfa/virtual_people/common/config.pb.h"

using bazel::tools::cpp::runfiles::Runfiles;
using ::wfa::ReadTextProtoFile;

namespace wfa_virtual_people {
namespace {

struct Targets {
  std::vector<std::string> protoDependecies;
  std::string name;
  std::string output;
  std::string golden;
  std::string proto;
  std::string protoType;
};

// Returns a list of Targets parsed from the input config.
// Additionally, locates the runfiles path for the given binary and executes the
// binary with its given binary_parameters from the input config.
std::vector<Targets> ParseConfig(std::string path) {
  IntegrationTestList config;
  std::vector<Targets> targets;
  std::vector<std::string> protoDependencies;
  std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest());
  std::string name, output, golden, proto, protoType, rPath, execute;

  absl::Status readConfigStatus = ReadTextProtoFile(path, config);
  CHECK(readConfigStatus == absl::OkStatus())
      << "Read Config Status: " << readConfigStatus;

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
        if (!bp.golden().golden_path().empty() && open(bp.golden().golden_path().data(), O_RDONLY) != -1) {
          protoDependencies = std::vector<std::string>(
              bp.golden().proto_dependencies().begin(), bp.golden().proto_dependencies().end());
          protoDependencies.push_back(bp.golden().proto_path());
          output = bp.value();
          golden = bp.golden().golden_path();
          proto = bp.golden().proto_path();
          protoType = bp.golden().proto_type();
          targets.push_back(
              {protoDependencies, name, output, golden, proto, protoType});
        }
      }
      std::system(execute.c_str());
    }
  }

  return targets;
}

// Declares classes and variables used in the parameterized TEST_P.
class IntegrationTestParamaterizedFixture
    : public ::testing::TestWithParam<Targets> {
 protected:
  google::protobuf::DescriptorPool descriptorPoolUnderlay;
  google::protobuf::FileDescriptorProto fileDescriptorProto;
  google::protobuf::compiler::Parser parser;
  google::protobuf::util::MessageDifferencer messageDifferencer;
};

// Parses proto type to compare from input config and compares output textproto
// against golden textproto.
TEST_P(IntegrationTestParamaterizedFixture, Test) {
  Targets targets(GetParam());

  google::protobuf::DescriptorPool descriptorPool(&descriptorPoolUnderlay);

  descriptorPool.AllowUnknownDependencies();
  descriptorPoolUnderlay.AllowUnknownDependencies();

  std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest());

  for (std::string protoDependency : targets.protoDependecies) {
    std::string rPath = runfiles->Rlocation(protoDependency);

    int fd = open(rPath.data(), O_RDONLY);
    google::protobuf::io::FileInputStream fileInputStream(fd);
    CHECK(fileInputStream.GetErrno() == 0)
        << "Errno: " << fileInputStream.GetErrno();

    google::protobuf::io::Tokenizer tokenizer(&fileInputStream, NULL);
    fileInputStream.SetCloseOnDelete(true);
    fileDescriptorProto.set_name(protoDependency);
    parser.Parse(&tokenizer, &fileDescriptorProto);
    fileDescriptorProto.clear_options();
    if (protoDependency == targets.proto) {
      descriptorPool.BuildFile(fileDescriptorProto);
    } else {
      descriptorPoolUnderlay.BuildFile(fileDescriptorProto);
    }
  }

  google::protobuf::DynamicMessageFactory dynamicMessageFactory;

  const google::protobuf::FileDescriptor* fileDescriptor =
      descriptorPool.FindFileByName(targets.proto);
  const google::protobuf::Descriptor* descriptor =
      fileDescriptor->FindMessageTypeByName(targets.protoType);
  const google::protobuf::Message* immutableMessage =
      dynamicMessageFactory.GetPrototype(descriptor);

  std::unique_ptr<google::protobuf::Message> output =
      std::unique_ptr<google::protobuf::Message>(immutableMessage->New());
  absl::Status outputStatus = ReadTextProtoFile(targets.output, *output);
  CHECK(outputStatus == absl::OkStatus()) << "Output Status: " << outputStatus;

  std::unique_ptr<google::protobuf::Message> golden =
      std::unique_ptr<google::protobuf::Message>(immutableMessage->New());
  absl::Status goldenStatus = ReadTextProtoFile(targets.golden, *golden);
  CHECK(goldenStatus == absl::OkStatus()) << "Golden Status: " << goldenStatus;

  if (messageDifferencer.Equals(*output, *golden)) {
    SUCCEED();
  } else {
    FAIL() << std::system(("diff " + targets.output + " " + targets.golden).c_str());
  }
}

// Instantiates all test cases from the input config and assigns each one a
// name.
INSTANTIATE_TEST_SUITE_P(
    IntegrationTest, IntegrationTestParamaterizedFixture,
    ::testing::ValuesIn(
        ParseConfig("src/test/cc/wfa/virtual_people/training/util/"
                    "test_data/config.textproto")),
    [](const ::testing::TestParamInfo<
        IntegrationTestParamaterizedFixture::ParamType>& info) {
      std::string name = info.param.name;
      return name;
    });

}  // namespace
}  // namespace wfa_virtual_people
