load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "load",
    hdrs = ["load.h"],
    linkopts = [
        "-lpthread",
        "-ltbb",
    ],
    deps = [
        "//runtime:column_data",
        "//runtime:column_index",
        "//runtime:date",
        "//runtime:enum",
    ],
)

cc_library(
    name = "builder",
    hdrs = ["builder.h"],
    deps = [
        ":vector_util",
        "//catalog",
        "//plan/expression",
        "//plan/expression:arithmetic_expression",
        "//plan/expression:case_expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:conversion_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "//plan/operator",
        "//plan/operator:group_by_aggregate_operator",
        "//plan/operator:hash_join_operator",
        "//plan/operator:order_by_operator",
        "//plan/operator:output_operator",
        "//plan/operator:scan_operator",
        "//plan/operator:select_operator",
        "//plan/operator:skinner_join_operator",
        "//runtime:date",
        "@re2",
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
    name = "vector_util",
    hdrs = ["vector_util.h"],
)

cc_library(
    name = "visitor",
    hdrs = ["visitor.h"],
)

cc_library(
    name = "profiler",
    hdrs = ["profiler.h"],
    deps = [
        ":profile_map_generator",
    ],
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
        ":profiler",
        ":test_util",
        "//compile:query_translator",
        "//execution:executable_query",
        "//plan/operator:output_operator",
    ],
)

cc_library(
    name = "test_util",
    hdrs = ["test_util.h"],
    deps = [
        "//compile:query_translator",
        "//execution:executable_query",
        "//plan/operator:operator_schema",
    ],
)
