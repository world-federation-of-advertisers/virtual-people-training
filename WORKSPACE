workspace(name = "virtual_people_training")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_google_absl_py",
    sha256 = "0d3aa64ef42ef592d5cf12060082397d01291bfeb1ac7b6d9dcfb32a07fff311",
    strip_prefix = "abseil-py-9954557f9df0b346a57ff82688438c55202d2188",
    urls = [
        "https://github.com/abseil/abseil-py/archive/9954557f9df0b346a57ff82688438c55202d2188.tar.gz",
    ],
)

# Six is named six_archive to match what absl_py expects.
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

load("@rules_python//python:pip.bzl", "pip_install")

pip_install(
    name = "pip_dependencies",
    requirements = "requirements.txt",
)

# Common-cpp
http_archive(
    name = "wfa_common_cpp",
    sha256 = "a964e147d4516ec40560531ad5fd654bd4aef37fc05c58d44ad1749b2dc466dc",
    strip_prefix = "common-cpp-0.6.0",
    url = "https://github.com/world-federation-of-advertisers/common-cpp/archive/refs/tags/v0.6.0.tar.gz",
)

load("@wfa_common_cpp//build:common_cpp_repositories.bzl", "common_cpp_repositories")

common_cpp_repositories()

load("@wfa_common_cpp//build:common_cpp_deps.bzl", "common_cpp_deps")

common_cpp_deps()

# Virtual-people-common
http_archive(
    name = "virtual_people_common",
    sha256 = "af8bb0ae29dab12566ce5ccafed93009e85eebbafa3a434deeac0b332377b3c3",
    strip_prefix = "virtual-people-common-7706e563737078d1921ed75a321112191a452250",
    url = "https://github.com/world-federation-of-advertisers/virtual-people-common/archive/7706e563737078d1921ed75a321112191a452250.tar.gz",
)
