load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "expression_visitor",
    hdrs = ["expression_visitor.h"],
    deps = [],
)

cc_library(
    name = "expression",
    hdrs = ["expression.h"],
    deps = [
        ":expression_visitor",
        "@json",
    ],
)

cc_library(
    name = "literal_expression",
    srcs = ["literal_expression.cc"],
    hdrs = ["literal_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
    ],
)

cc_library(
    name = "column_ref_expression",
    srcs = ["column_ref_expression.cc"],
    hdrs = ["column_ref_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
    ],
)

cc_library(
    name = "comparison_expression",
    srcs = ["comparison_expression.cc"],
    hdrs = ["comparison_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
        "@magic_enum",
    ],
)

cc_library(
    name = "aggregate_expression",
    srcs = ["aggregate_expression.cc"],
    hdrs = ["aggregate_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
        "@magic_enum",
    ],
)
