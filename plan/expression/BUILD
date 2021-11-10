load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "expression_visitor",
    hdrs = ["expression_visitor.h"],
    deps = [],
)

cc_library(
    name = "expression",
    srcs = ["expression.cc"],
    hdrs = ["expression.h"],
    deps = [
        ":expression_visitor",
        "//catalog:sql_type",
        "//util:vector_util",
        "@json",
        "@magic_enum",
    ],
)

cc_library(
    name = "literal_expression",
    srcs = ["literal_expression.cc"],
    hdrs = ["literal_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@absl//absl/time",
        "@json",
        "@magic_enum",
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
        "@magic_enum",
    ],
)

cc_library(
    name = "virtual_column_ref_expression",
    srcs = ["virtual_column_ref_expression.cc"],
    hdrs = ["virtual_column_ref_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
        "@magic_enum",
    ],
)

cc_library(
    name = "case_expression",
    srcs = ["case_expression.cc"],
    hdrs = ["case_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
        "@magic_enum",
    ],
)

cc_library(
    name = "arithmetic_expression",
    srcs = ["arithmetic_expression.cc"],
    hdrs = ["arithmetic_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
        "@magic_enum",
        "@re2",
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

cc_library(
    name = "conversion_expression",
    srcs = ["conversion_expression.cc"],
    hdrs = ["conversion_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
        "@magic_enum",
    ],
)

cc_library(
    name = "extract_expression",
    srcs = ["extract_expression.cc"],
    hdrs = ["extract_expression.h"],
    deps = [
        ":expression",
        ":expression_visitor",
        "@json",
        "@magic_enum",
    ],
)
