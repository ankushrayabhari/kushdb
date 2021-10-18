load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "hyperd",
    srcs = ["lib/hyper/hyperd"],
)

cc_library(
    name = "hyper",
    srcs = ["lib/libtableauhyperapi.so"],
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ]),
    strip_include_prefix = "include",
)
