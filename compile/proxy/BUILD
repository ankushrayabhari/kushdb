load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "struct",
    srcs = ["struct.cc"],
    hdrs = ["struct.h"],
    deps = [
        "//catalog:sql_type",
        "//compile/proxy/control_flow:if",
        "//compile/proxy/value:ir_value",
        "//compile/proxy/value:sql_value",
        "//khir:program_builder",
    ],
)

cc_library(
    name = "hash_table",
    srcs = ["hash_table.cc"],
    hdrs = ["hash_table.h"],
    deps = [
        ":struct",
        ":vector",
        "//compile/proxy/control_flow:loop",
        "//khir:program_builder",
        "//runtime:hash_table",
        "//util:vector_util",
    ],
)

cc_library(
    name = "vector",
    srcs = ["vector.cc"],
    hdrs = ["vector.h"],
    deps = [
        ":struct",
        "//catalog:sql_type",
        "//compile/proxy/control_flow:if",
        "//compile/proxy/value:ir_value",
        "//khir:program_builder",
        "//runtime:vector",
    ],
)

cc_library(
    name = "tuple_idx_table",
    srcs = ["tuple_idx_table.cc"],
    hdrs = ["tuple_idx_table.h"],
    deps = [
        "//compile/proxy/control_flow:loop",
        "//compile/proxy/value:ir_value",
        "//khir:program_builder",
        "//runtime:tuple_idx_table",
        "//util:vector_util",
    ],
)

cc_library(
    name = "function",
    srcs = ["function.cc"],
    hdrs = ["function.h"],
    deps = [
        ":struct",
        "//compile/proxy/value:ir_value",
        "//khir:program_builder",
    ],
)

cc_library(
    name = "skinner_join_executor",
    srcs = ["skinner_join_executor.cc"],
    hdrs = ["skinner_join_executor.h"],
    deps = [
        ":tuple_idx_table",
        "//compile/proxy/value:ir_value",
        "//compile/translators:recompiling_join_translator",
        "//khir:program_builder",
        "//runtime:skinner_join_executor",
    ],
)

cc_library(
    name = "column_data",
    srcs = ["column_data.cc"],
    hdrs = ["column_data.h"],
    deps = [
        "//catalog:sql_type",
        "//compile/proxy/value:ir_value",
        "//compile/proxy/value:sql_value",
        "//khir:program_builder",
        "//runtime:column_data",
    ],
)

cc_library(
    name = "column_index",
    srcs = ["column_index.cc"],
    hdrs = ["column_index.h"],
    deps = [
        "//catalog:sql_type",
        "//compile/proxy/value:ir_value",
        "//khir:program_builder",
        "//runtime:column_index",
    ],
)
