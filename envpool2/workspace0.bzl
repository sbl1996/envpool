# Copyright 2021 Garena Online Private Limited
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

"""EnvPool workspace initialization, this is loaded in WORKSPACE."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def workspace():
    """Load requested packages."""
    # we cannot upgrade rules_python because it requires requirements_lock.txt after 0.13.0
    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "b593d13bb43c94ce94b483c2858e53a9b811f6f10e1e0eedc61073bd90e58d9c",
        strip_prefix = "rules_python-0.12.0",
        urls = [
            "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.12.0.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/bazelbuild/rules_python/0.12.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "rules_foreign_cc",
        sha256 = "476303bd0f1b04cc311fc258f1708a5f6ef82d3091e53fd1977fa20383425a6a",
        strip_prefix = "rules_foreign_cc-0.10.1",
        urls = [
            "https://github.com/bazelbuild/rules_foreign_cc/archive/0.10.1.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/bazelbuild/rules_foreign_cc/0.10.1.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "pybind11_bazel",
        sha256 = "2c466c9b3cca7852b47e0785003128984fcf0d5d61a1a2e4c5aceefd935ac220",
        strip_prefix = "pybind11_bazel-2.11.1",
        urls = [
            "https://github.com/pybind/pybind11_bazel/archive/refs/tags/v2.11.1.zip",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/pybind/pybind11_bazel/v2.11.1.zip",
        ],
    )

    maybe(
        http_archive,
        name = "pybind11",
        build_file = "@pybind11_bazel//:pybind11.BUILD",
        sha256 = "d475978da0cdc2d43b73f30910786759d593a9d8ee05b1b6846d1eb16c6d2e0c",
        strip_prefix = "pybind11-2.11.1",
        urls = [
            "https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/pybind/pybind11/v2.11.1.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_absl",
        sha256 = "497ebdc3a4885d9209b9bd416e8c3f71e7a1fb8af249f6c2a80b7cbeefcd7e21",
        strip_prefix = "abseil-cpp-20230802.1",
        urls = [
            "https://github.com/abseil/abseil-cpp/archive/refs/tags/20230802.1.zip",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/abseil/abseil-cpp/20230802.1.zip",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_gflags_gflags",
        sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
        strip_prefix = "gflags-2.2.2",
        urls = [
            "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/gflags/gflags/v2.2.2.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_google_glog",
        sha256 = "122fb6b712808ef43fbf80f75c52a21c9760683dae470154f02bddfc61135022",
        strip_prefix = "glog-0.6.0",
        urls = [
            "https://github.com/google/glog/archive/v0.6.0.zip",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/google/glog/v0.6.0.zip",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7",
        strip_prefix = "googletest-1.14.0",
        urls = [
            "https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/google/googletest/v1.14.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "glibc_version_header",
        sha256 = "57db74f933b7a9ea5c653498640431ce0e52aaef190d6bb586711ec4f8aa2b9e",
        strip_prefix = "glibc_version_header-0.1/version_headers/",
        urls = [
            "https://github.com/wheybags/glibc_version_header/archive/refs/tags/0.1.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/wheybags/glibc_version_header/0.1.tar.gz",
        ],
        build_file = "//third_party/glibc_version_header:glibc_version_header.BUILD",
    )

    maybe(
        http_archive,
        name = "concurrentqueue",
        sha256 = "87fbc9884d60d0d4bf3462c18f4c0ee0a9311d0519341cac7cbd361c885e5281",
        strip_prefix = "concurrentqueue-1.0.4",
        urls = [
            "https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.4.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/cameron314/concurrentqueue/v1.0.4.tar.gz",
        ],
        build_file = "//third_party/concurrentqueue:concurrentqueue.BUILD",
    )

    maybe(
        http_archive,
        name = "threadpool",
        sha256 = "18854bb7ecc1fc9d7dda9c798a1ef0c81c2dd331d730c76c75f648189fa0c20f",
        strip_prefix = "ThreadPool-9a42ec1329f259a5f4881a291db1dcb8f2ad9040",
        urls = [
            "https://github.com/progschj/ThreadPool/archive/9a42ec1329f259a5f4881a291db1dcb8f2ad9040.zip",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/progschj/ThreadPool/9a42ec1329f259a5f4881a291db1dcb8f2ad9040.zip",
        ],
        build_file = "//third_party/threadpool:threadpool.BUILD",
    )

    maybe(
        http_archive,
        name = "com_github_nelhage_rules_boost",
        # sha256 = "2215e6910eb763a971b1f63f53c45c0f2b7607df38c96287666d94d954da8cdc",
        strip_prefix = "rules_boost-e60cf50996da9fe769b6e7a31b88c54966ecb191",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/e60cf50996da9fe769b6e7a31b88c54966ecb191.tar.gz",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/nelhage/rules_boost/e60cf50996da9fe769b6e7a31b88c54966ecb191.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "boost",
        build_file = "@com_github_nelhage_rules_boost//:boost.BUILD",
        patch_cmds = ["rm -f doc/pdf/BUILD"],
        sha256 = "6478edfe2f3305127cffe8caf73ea0176c53769f4bf1585be237eb30798c3b8e",
        strip_prefix = "boost_1_83_0",
        urls = [
            "https://boostorg.jfrog.io/artifactory/main/release/1.83.0/source/boost_1_83_0.tar.bz2",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/boost/boost_1_83_0.tar.bz2",
        ],
    )

    maybe(
        http_archive,
        name = "bazel_clang_tidy",
        sha256 = "ec8c5bf0c02503b928c2e42edbd15f75e306a05b2cae1f34a7bc84724070b98b",
        strip_prefix = "bazel_clang_tidy-783aa523aafb4a6798a538c61e700b6ed27975a7",
        urls = [
            "https://github.com/erenon/bazel_clang_tidy/archive/783aa523aafb4a6798a538c61e700b6ed27975a7.zip",
            "https://ml.cs.tsinghua.edu.cn/~jiayi/envpool/erenon/bazel_clang_tidy/783aa523aafb4a6798a538c61e700b6ed27975a7.zip",
        ],
    )

workspace0 = workspace
