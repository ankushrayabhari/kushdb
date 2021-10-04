load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ir_value",
    srcs = [
        "bool.cc",
        "float.cc",
        "int.cc",
        "printer.cc",
        "string.cc",
    ],
    hdrs = [
        "ir_value.h",
    ],
    deps = [
        ":numeric",
        "//khir:program_builder",
        "//plan/expression:binary_arithmetic_expression",
        "//runtime:printer",
        "//runtime:string",
    ],
)

cc_library(
    name = "numeric",
    hdrs = ["numeric.h"],
    deps = [
        "//plan/expression:binary_arithmetic_expression",
    ],
)
