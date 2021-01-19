load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "schema_values",
    hdrs = ["schema_values.h"],
    deps = [
        "//compile/proxy:value",
    ],
)

cc_library(
    name = "operator_translator",
    srcs = ["operator_translator.cc"],
    hdrs = ["operator_translator.h"],
    deps = [
        ":schema_values",
        "//util:vector_util",
    ],
)

cc_library(
    name = "scan_translator",
    srcs = ["scan_translator.cc"],
    hdrs = ["scan_translator.h"],
    deps = [
        ":operator_translator",
        "//compile/llvm:llvm_program",
        "//plan:scan_operator",
    ],
)

cc_library(
    name = "output_translator",
    srcs = ["output_translator.cc"],
    hdrs = ["output_translator.h"],
    deps = [
        ":operator_translator",
        "//compile/llvm:llvm_program",
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
        "//compile/llvm:llvm_program",
        "//plan:select_operator",
    ],
)

cc_library(
    name = "cross_product_translator",
    srcs = ["cross_product_translator.cc"],
    hdrs = ["cross_product_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile/llvm:llvm_program",
        "//plan:cross_product_operator",
    ],
)

cc_library(
    name = "hash_join_translator",
    srcs = ["hash_join_translator.cc"],
    hdrs = ["hash_join_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile/llvm:llvm_program",
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
        "//compile/llvm:llvm_program",
        "//plan:group_by_aggregate_operator",
    ],
)

cc_library(
    name = "order_by_translator",
    srcs = ["order_by_translator.cc"],
    hdrs = ["order_by_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile/llvm:llvm_program",
        "//plan:order_by_operator",
    ],
)

cc_library(
    name = "translator_factory",
    srcs = ["translator_factory.cc"],
    hdrs = ["translator_factory.h"],
    deps = [
        ":cross_product_translator",
        ":group_by_aggregate_translator",
        ":hash_join_translator",
        ":operator_translator",
        ":order_by_translator",
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
        "//compile/llvm:llvm_program",
        "//compile/proxy:boolean",
        "//compile/proxy:float",
        "//compile/proxy:if",
        "//compile/proxy:integer",
        "//compile/proxy:value",
        "//plan/expression:aggregate_expression",
        "//plan/expression:binary_arithmetic_expression",
        "//plan/expression:case_expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:expression_visitor",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "//util:visitor",
        "@absl//absl/container:flat_hash_map",
    ],
)
