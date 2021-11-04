load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "planner",
    srcs = ["planner.cc"],
    hdrs = ["planner.h"],
    deps = [
        "//catalog:catalog_manager",
        "//parse/expression",
        "//parse/expression:aggregate_expression",
        "//parse/expression:arithmetic_expression",
        "//parse/expression:case_expression",
        "//parse/expression:column_ref_expression",
        "//parse/expression:in_expression",
        "//parse/expression:literal_expression",
        "//parse/statement:select_statement",
        "//plan/expression",
        "//plan/expression:aggregate_expression",
        "//plan/expression:arithmetic_expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "//plan/operator",
        "//plan/operator:group_by_aggregate_operator",
        "//plan/operator:scan_operator",
        "//plan/operator:select_operator",
        "//plan/operator:skinner_join_operator",
        "//util:vector_util",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
    ],
)
