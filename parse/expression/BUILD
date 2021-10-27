load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "expression",
    srcs = ["expression.cc"],
    hdrs = ["expression.h"],
    deps = [],
)

cc_library(
    name = "column_ref_expression",
    srcs = ["column_ref_expression.cc"],
    hdrs = ["column_ref_expression.h"],
    deps = [
        ":expression",
    ],
)

cc_library(
    name = "literal_expression",
    srcs = ["literal_expression.cc"],
    hdrs = ["literal_expression.h"],
    deps = [
        ":expression",
        "@absl//absl/time",
    ],
)

cc_library(
    name = "arithmetic_expression",
    srcs = ["arithmetic_expression.cc"],
    hdrs = ["arithmetic_expression.h"],
    deps = [
        ":expression",
    ],
)

cc_library(
    name = "comparison_expression",
    srcs = ["comparison_expression.cc"],
    hdrs = ["comparison_expression.h"],
    deps = [
        ":expression",
    ],
)

cc_library(
    name = "in_expression",
    srcs = ["in_expression.cc"],
    hdrs = ["in_expression.h"],
    deps = [
        ":expression",
    ],
)

cc_library(
    name = "case_expression",
    srcs = ["case_expression.cc"],
    hdrs = ["case_expression.h"],
    deps = [
        ":expression",
    ],
)
