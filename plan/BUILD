load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "operator_visitor",
    hdrs = ["operator_visitor.h"],
    deps = [],
)

cc_library(
    name = "operator_schema",
    srcs = ["operator_schema.cc"],
    hdrs = ["operator_schema.h"],
    deps = [
        "//catalog:sql_type",
        "//plan/expression",
        "@json",
        "@magic_enum",
    ],
)

cc_library(
    name = "operator",
    srcs = ["operator.cc"],
    hdrs = ["operator.h"],
    deps = [
        ":operator_schema",
        ":operator_visitor",
        "//plan/expression",
        "//plan/expression:column_ref_expression",
        "//util:vector_util",
        "@json",
    ],
)
