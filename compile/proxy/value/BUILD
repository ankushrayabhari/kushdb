load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ir_value",
    srcs = [
        "bool.cc",
        "date.cc",
        "float.cc",
        "int.cc",
        "printer.cc",
        "string.cc",
    ],
    hdrs = [
        "ir_value.h",
    ],
    deps = [
        "//khir:program_builder",
        "//runtime:printer",
        "//runtime:string",
    ],
)

cc_library(
    name = "sql_value",
    srcs = ["sql_value.cc"],
    hdrs = ["sql_value.h"],
    deps = [
        ":ir_value",
        "//catalog:sql_type",
        "//khir:program_builder",
        "//plan/expression:binary_arithmetic_expression",
    ],
)
