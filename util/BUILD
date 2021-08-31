load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "builder",
    hdrs = ["builder.h"],
    deps = [
        ":vector_util",
        "//catalog",
        "//compile:query_translator",
        "//plan:group_by_aggregate_operator",
        "//plan:hash_join_operator",
        "//plan:operator",
        "//plan:order_by_operator",
        "//plan:output_operator",
        "//plan:scan_operator",
        "//plan:select_operator",
        "//plan:skinner_join_operator",
        "//plan/expression",
        "//plan/expression:binary_arithmetic_expression",
        "//plan/expression:case_expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:conversion_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
    ],
)

cc_library(
    name = "profile_map_generator",
    srcs = ["profile_map_generator.cc"],
    hdrs = ["profile_map_generator.h"],
)

cc_library(
    name = "union_find",
    srcs = ["union_find.cc"],
    hdrs = ["union_find.h"],
)

cc_library(
    name = "hash_util",
    hdrs = ["hash_util.h"],
)

cc_library(
    name = "vector_util",
    hdrs = ["vector_util.h"],
)

cc_library(
    name = "visitor",
    hdrs = ["visitor.h"],
)

cc_library(
    name = "nn",
    hdrs = ["nn.h"],
    deps = [
        "@nn",
    ],
)

cc_library(
    name = "time_execute",
    hdrs = ["time_execute.h"],
    deps = [
        "//compile:query_translator",
    ],
)
