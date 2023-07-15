# Copyright 2023 The Cross-Media Measurement Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Adds external repos necessary for virtual_people_training.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def virtual_people_training_repositories():
    """Imports all direct dependencies for virtual_people_training."""
    http_archive(
        name = "com_google_absl_py",
        sha256 = "0d3aa64ef42ef592d5cf12060082397d01291bfeb1ac7b6d9dcfb32a07fff311",
        strip_prefix = "abseil-py-9954557f9df0b346a57ff82688438c55202d2188",
        urls = [
            "https://github.com/abseil/abseil-py/archive/9954557f9df0b346a57ff82688438c55202d2188.tar.gz",
        ],
    )

    http_archive(
        name = "six_archive",
        build_file = "@com_google_absl_py//third_party:six.BUILD",
        sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
        strip_prefix = "six-1.10.0",
        urls = [
            "http://mirror.bazel.build/pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
            "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
        ],
    )

    http_archive(
        name = "rules_python",
        sha256 = "934c9ceb552e84577b0faf1e5a2f0450314985b4d8712b2b70717dc679fdc01b",
        urls = ["https://github.com/bazelbuild/rules_python/releases/download/0.3.0/rules_python-0.3.0.tar.gz"],
    )

    http_archive(
        name = "wfa_common_cpp",
        sha256 = "60e9c808d55d14be65347cab008b8bd4f8e2dd8186141609995333bc75fc08ce",
        strip_prefix = "common-cpp-0.8.0",
        url = "https://github.com/world-federation-of-advertisers/common-cpp/archive/refs/tags/v0.8.0.tar.gz",
    )

    http_archive(
        name = "wfa_common_jvm",
        sha256 = "ad623ee3b1893b47fc6c86d6b1c90ea1f46a44bdf502a1847518f6769597c5cf",
        strip_prefix = "common-jvm-0.45.0",
        url = "https://github.com/world-federation-of-advertisers/common-jvm/archive/refs/tags/v0.45.0.tar.gz",
    )

    http_archive(
        name = "wfa_virtual_people_common",
        sha256 = "9cd92c85a86c86c7228e860969bc31148049ece3c6ce9f1ce2047e907c3187ff",
        strip_prefix = "virtual-people-common-0.2.4",
        url = "https://github.com/world-federation-of-advertisers/virtual-people-common/archive/refs/tags/v0.2.4.tar.gz",
    )
