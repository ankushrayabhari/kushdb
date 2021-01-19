load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "value",
    hdrs = ["value.h"],
    deps = [
        "//compile:program_builder",
    ],
)

cc_library(
    name = "boolean",
    hdrs = ["boolean.h"],
    deps = [
        ":value",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "integer",
    hdrs = ["integer.h"],
    deps = [
        ":boolean",
        ":value",
        "//compile/llvm:llvm_program",
    ],
)

cc_library(
    name = "float",
    hdrs = ["float.h"],
    deps = [
        ":boolean",
        ":value",
        "//compile/llvm:llvm_program",
    ],
)

cc_library(
    name = "if",
    hdrs = ["if.h"],
    deps = [
        ":boolean",
        "//compile:program",
    ],
)

cc_library(
    name = "loop",
    hdrs = ["loop.h"],
    deps = [
        ":boolean",
        "//compile:program",
    ],
)
