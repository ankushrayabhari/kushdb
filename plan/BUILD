load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "operator_visitor",
    hdrs = ["operator_visitor.h"],
    deps = [],
)

cc_library(
    name = "operator",
    srcs = ["operator.cc"],
    hdrs = ["operator.h"],
    deps = [
        ":operator_visitor",
        "//plan/expression",
        "//plan/expression:column_ref_expression",
        "@json",
    ],
)
