load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Common-cpp
http_archive(
    name = "wfa_common_cpp",
    sha256 = "e0e1f5eed832ef396109354a64c6c1306bf0fb5ea0b449ce6ee1e8edc6fe279d",
    strip_prefix = "common-cpp-43c75acc3394e19bcfd2cfe8e8e2454365d26d60",
    url = "https://github.com/world-federation-of-advertisers/common-cpp/archive/43c75acc3394e19bcfd2cfe8e8e2454365d26d60.tar.gz",
)

load("@wfa_common_cpp//build:deps.bzl", "common_cpp_deps")

common_cpp_deps()
