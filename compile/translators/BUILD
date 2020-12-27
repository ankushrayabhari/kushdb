load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "operator_translator",
    srcs = ["operator_translator.cc"],
    hdrs = ["operator_translator.h"],
    deps = [
        "//compile:schema_values",
        "//util:vector_util",
    ],
)

cc_library(
    name = "scan_translator",
    srcs = ["scan_translator.cc"],
    hdrs = ["scan_translator.h"],
    deps = [
        ":operator_translator",
        "//compile:compilation_context",
        "//compile:types",
        "//plan:scan_operator",
    ],
)

cc_library(
    name = "output_translator",
    srcs = ["output_translator.cc"],
    hdrs = ["output_translator.h"],
    deps = [
        ":operator_translator",
        "//compile:compilation_context",
        "//plan:output_operator",
    ],
)

cc_library(
    name = "select_translator",
    srcs = ["select_translator.cc"],
    hdrs = ["select_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile:compilation_context",
        "//compile:types",
        "//plan:select_operator",
    ],
)

cc_library(
    name = "hash_join_translator",
    srcs = ["hash_join_translator.cc"],
    hdrs = ["hash_join_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile:compilation_context",
        "//compile:types",
        "//plan:hash_join_operator",
    ],
)

cc_library(
    name = "group_by_aggregate_translator",
    srcs = ["group_by_aggregate_translator.cc"],
    hdrs = ["group_by_aggregate_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile:compilation_context",
        "//compile:types",
        "//plan:group_by_aggregate_operator",
    ],
)

cc_library(
    name = "translator_factory",
    srcs = ["translator_factory.cc"],
    hdrs = ["translator_factory.h"],
    deps = [
        ":group_by_aggregate_translator",
        ":hash_join_translator",
        ":operator_translator",
        ":output_translator",
        ":scan_translator",
        ":select_translator",
        "//plan:hash_join_operator",
        "//plan:operator",
        "//plan:output_operator",
        "//plan:scan_operator",
        "//plan:select_operator",
        "//util:visitor",
    ],
)

cc_library(
    name = "expression_translator",
    srcs = ["expression_translator.cc"],
    hdrs = ["expression_translator.h"],
    deps = [
        ":operator_translator",
        "//compile:compilation_context",
        "//plan/expression:aggregate_expression",
        "//plan/expression:arithmetic_expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:comparison_expression",
        "//plan/expression:expression_visitor",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "@absl//absl/container:flat_hash_map",
    ],
)
