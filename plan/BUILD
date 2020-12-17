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

cc_library(
    name = "scan_operator",
    srcs = ["scan_operator.cc"],
    hdrs = ["scan_operator.h"],
    deps = [
        ":operator",
        ":operator_schema",
        ":operator_visitor",
        "@json",
    ],
)

cc_library(
    name = "output_operator",
    srcs = ["output_operator.cc"],
    hdrs = ["output_operator.h"],
    deps = [
        ":operator",
        ":operator_schema",
        ":operator_visitor",
        "@json",
    ],
)

cc_library(
    name = "select_operator",
    srcs = ["select_operator.cc"],
    hdrs = ["select_operator.h"],
    deps = [
        ":operator",
        ":operator_schema",
        ":operator_visitor",
        "@json",
    ],
)

cc_library(
    name = "hash_join_operator",
    srcs = ["hash_join_operator.cc"],
    hdrs = ["hash_join_operator.h"],
    deps = [
        ":operator",
        ":operator_schema",
        ":operator_visitor",
        "//plan/expression:column_ref_expression",
        "@json",
    ],
)
