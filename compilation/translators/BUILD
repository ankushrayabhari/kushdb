load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "operator_translator",
    srcs = ["operator_translator.cc"],
    hdrs = ["operator_translator.h"],
    deps = [
        "//compilation:schema_values",
    ],
)

cc_library(
    name = "scan_translator",
    srcs = ["scan_translator.cc"],
    hdrs = ["scan_translator.h"],
    deps = [
        ":operator_translator",
        "//compilation:compilation_context",
        "//plan:operator",
        "//plan:sql_type",
    ],
)

cc_library(
    name = "output_translator",
    srcs = ["output_translator.cc"],
    hdrs = ["output_translator.h"],
    deps = [
        ":operator_translator",
        "//compilation:compilation_context",
        "//plan:operator",
    ],
)

cc_library(
    name = "select_translator",
    srcs = ["select_translator.cc"],
    hdrs = ["select_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compilation:compilation_context",
        "//plan:operator",
    ],
)

cc_library(
    name = "hash_join_translator",
    srcs = ["hash_join_translator.cc"],
    hdrs = ["hash_join_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compilation:compilation_context",
        "//plan:operator",
    ],
)

cc_library(
    name = "translator_factory",
    srcs = ["translator_factory.cc"],
    hdrs = ["translator_factory.h"],
    deps = [
        ":hash_join_translator",
        ":operator_translator",
        ":output_translator",
        ":scan_translator",
        ":select_translator",
        "//util:visitor",
    ],
)

cc_library(
    name = "expression_translator",
    srcs = ["expression_translator.cc"],
    hdrs = ["expression_translator.h"],
    deps = [
        ":operator_translator",
        "//compilation:compilation_context",
        "//plan/expression:column_ref_expression",
        "//plan/expression:comparison_expression",
        "//plan/expression:expression_visitor",
        "//plan/expression:literal_expression",
    ],
)
