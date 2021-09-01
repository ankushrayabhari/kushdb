load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "bazel_skylib",
    sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
    urls = [
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

http_archive(
    name = "rules_cc",
    sha256 = "9a446e9dd9c1bb180c86977a8dc1e9e659550ae732ae58bd2e8fd51e15b2c91d",
    strip_prefix = "rules_cc-262ebec3c2296296526740db4aefce68c80de7fa",
    urls = ["https://github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip"],
)

http_archive(
    name = "com_google_googletest",
    sha256 = "5cf189eb6847b4f8fc603a3ffff3b0771c08eec7dd4bd961bfd45477dd13eb73",
    strip_prefix = "googletest-609281088cfefc76f9d0ce82e1ff6c30cc3591e5",
    urls = ["https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip"],
)

http_archive(
    name = "absl",
    sha256 = "aabf6c57e3834f8dc3873a927f37eaf69975d4b28117fc7427dfb1c661542a87",
    strip_prefix = "abseil-cpp-98eb410c93ad059f9bba1bf43f5bb916fc92a5ea",
    urls = ["https://github.com/abseil/abseil-cpp/archive/98eb410c93ad059f9bba1bf43f5bb916fc92a5ea.zip"],
)

http_archive(
    name = "magic_enum",
    sha256 = "f5f377cb861eea7ea541716d5bd64bbbd664f1ee4331d800023a3416ef5e1853",
    strip_prefix = "magic_enum-0.7.0",
    urls = ["https://github.com/Neargye/magic_enum/archive/v0.7.0.zip"],
)

http_archive(
    name = "json",
    build_file = "@//third_party:json.BUILD",
    sha256 = "a88449d68aab8d027c5beefe911ba217f5ffcc0686ae1793d37f3d20698b37c6",
    strip_prefix = "json-3.9.1",
    urls = ["https://github.com/nlohmann/json/archive/v3.9.1.zip"],
)

http_archive(
    name = "googletest",
    sha256 = "94c634d499558a76fa649edb13721dce6e98fb1e7018dfaeba3cd7a083945e91",
    strip_prefix = "googletest-release-1.10.0",
    urls = ["https://github.com/google/googletest/archive/release-1.10.0.zip"],
)

http_archive(
    name = "type_safe",
    build_file = "@//third_party:type_safe.BUILD",
    sha256 = "1c941f7ecd5e17e80773a2d8c9c905f552cc80417f8006ade7e9fa3525ff1b55",
    strip_prefix = "type_safe-0.2.1",
    urls = ["https://github.com/foonathan/type_safe/archive/v0.2.1.zip"],
)

http_archive(
    name = "debug_assert",
    build_file = "@//third_party:debug_assert.BUILD",
    sha256 = "c0871c7bb8d7e0f36cfae9a5ba526ea0c0fcaa41203ffb4280de27cb030e7383",
    strip_prefix = "debug_assert-1.3.3",
    urls = ["https://github.com/foonathan/debug_assert/archive/refs/tags/v1.3.3.zip"],
)

http_archive(
    name = "nn",
    build_file = "@//third_party:nn.BUILD",
    sha256 = "17662cfdc8ceb63272ad9c8586fd8b3620cf087edefce7d0d64deae28b7dcf4e",
    strip_prefix = "nn-master",
    urls = ["https://github.com/dropbox/nn/archive/refs/heads/master.zip"],
)

# Replace with the LLVM commit you want to use.
LLVM_COMMIT = "f2694500c2b50dd712e941b276acf68733778b29"

# The easiest way to calculate this for a new commit is to set it to empty and
# then run a bazel build and it will report the digest necessary to cache the
# archive and make the build reproducible.
LLVM_SHA256 = "16c710d2f77426f1e607967048b491a8b3e6f72c762ec1a6342025d12960dd13"

http_archive(
    name = "llvm-raw",
    build_file_content = "# empty",
    sha256 = LLVM_SHA256,
    strip_prefix = "llvm-project-" + LLVM_COMMIT,
    urls = ["https://github.com/llvm/llvm-project/archive/{commit}.tar.gz".format(commit = LLVM_COMMIT)],
)

load("@llvm-raw//utils/bazel:configure.bzl", "llvm_configure", "llvm_disable_optional_support_deps")

llvm_configure(name = "llvm-project")

# Disables optional dependencies for Support like zlib and terminfo. You may
# instead want to configure them using the macros in the corresponding bzl
# files.
llvm_disable_optional_support_deps()

http_archive(
    name = "asmjit",
    build_file = "@//third_party:asmjit.BUILD",
    sha256 = "33c1a131583850284ece021f78e945840a5183eb1b142a8b439c3ccaf963c16b",
    strip_prefix = "asmjit-master",
    urls = ["https://github.com/ankushrayabhari/asmjit/archive/refs/heads/master.zip"],
)

http_archive(
    name = "re2",
    sha256 = "40593c03cf7f309facc2cf8eea7d5a5c84d395ebfacbc2cedab75f67983a4e43",
    strip_prefix = "re2-2021-08-01",
    urls = ["https://github.com/google/re2/archive/refs/tags/2021-08-01.zip"],
)
