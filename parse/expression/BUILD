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
