load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_cc",
    sha256 = "9a446e9dd9c1bb180c86977a8dc1e9e659550ae732ae58bd2e8fd51e15b2c91d",
    strip_prefix = "rules_cc-262ebec3c2296296526740db4aefce68c80de7fa",
    urls = ["https://github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip"],
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
