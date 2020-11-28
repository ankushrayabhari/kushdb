load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cpp_program",
    srcs = ["cpp_program.cc"],
    hdrs = ["cpp_program.h"],
    deps = [],
)

cc_library(
    name = "cpp_translator",
    srcs = ["cpp_translator.cc"],
    hdrs = ["cpp_translator.h"],
    linkopts = ["-ldl"],
    deps = [
        ":cpp_program",
        "//catalog",
        "//plan:operator",
        "//plan/expression",
    ],
)
