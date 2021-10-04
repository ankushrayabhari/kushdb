load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "schema_values",
    srcs = ["schema_values.cc"],
    hdrs = ["schema_values.h"],
    deps = [
        "//compile/proxy/value",
    ],
)

cc_library(
    name = "operator_translator",
    srcs = ["operator_translator.cc"],
    hdrs = ["operator_translator.h"],
    deps = [
        ":schema_values",
        "//plan:operator",
        "//util:vector_util",
    ],
)

cc_library(
    name = "expression_translator",
    srcs = ["expression_translator.cc"],
    hdrs = ["expression_translator.h"],
    deps = [
        ":operator_translator",
        "//compile/proxy/control_flow:if",
        "//compile/proxy/value",
        "//khir:program_builder",
        "//plan/expression",
        "//plan/expression:aggregate_expression",
        "//plan/expression:binary_arithmetic_expression",
        "//plan/expression:case_expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:conversion_expression",
        "//plan/expression:expression_visitor",
        "//plan/expression:extract_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "//runtime:date_extractor",
        "//util:visitor",
    ],
)

cc_library(
    name = "scan_translator",
    srcs = ["scan_translator.cc"],
    hdrs = ["scan_translator.h"],
    deps = [
        ":operator_translator",
        "//catalog:sql_type",
        "//compile/proxy:column_data",
        "//compile/proxy/control_flow:loop",
        "//khir:program_builder",
        "//plan:scan_operator",
    ],
)

cc_library(
    name = "output_translator",
    srcs = ["output_translator.cc"],
    hdrs = ["output_translator.h"],
    deps = [
        ":operator_translator",
        "//compile/proxy/value",
        "//execution:pipeline",
        "//khir:program_builder",
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
        "//khir:program_builder",
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
        "//khir:program_builder",
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
        "//compile/proxy:hash_table",
        "//compile/proxy:struct",
        "//compile/proxy/control_flow:loop",
        "//compile/proxy/value",
        "//execution:pipeline",
        "//khir:program_builder",
        "//plan:hash_join_operator",
        "//util:vector_util",
    ],
)

cc_library(
    name = "predicate_column_collector",
    srcs = ["predicate_column_collector.cc"],
    hdrs = ["predicate_column_collector.h"],
    deps = [
        "//plan/expression",
        "//plan/expression:aggregate_expression",
        "//plan/expression:binary_arithmetic_expression",
        "//plan/expression:case_expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:conversion_expression",
        "//plan/expression:expression_visitor",
        "//plan/expression:extract_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "recompiling_join_translator",
    hdrs = ["recompiling_join_translator.h"],
    deps = [],
)

cc_library(
    name = "recompiling_skinner_join_translator",
    srcs = ["recompiling_skinner_join_translator.cc"],
    hdrs = ["recompiling_skinner_join_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        ":predicate_column_collector",
        ":recompiling_join_translator",
        ":scan_translator",
        "//compile:forward_declare",
        "//compile/proxy:column_index",
        "//compile/proxy:function",
        "//compile/proxy:skinner_join_executor",
        "//compile/proxy:struct",
        "//compile/proxy:tuple_idx_table",
        "//compile/proxy:vector",
        "//compile/proxy/control_flow:if",
        "//compile/proxy/control_flow:loop",
        "//compile/proxy/value",
        "//execution:pipeline",
        "//khir:backend",
        "//khir:compilation_cache",
        "//khir:program_builder",
        "//plan:skinner_join_operator",
        "//util:vector_util",
        "@absl//absl/container:btree",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "permutable_skinner_join_translator",
    srcs = ["permutable_skinner_join_translator.cc"],
    hdrs = ["permutable_skinner_join_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        ":predicate_column_collector",
        ":scan_translator",
        "//compile/proxy:column_index",
        "//compile/proxy:function",
        "//compile/proxy:skinner_join_executor",
        "//compile/proxy:struct",
        "//compile/proxy:tuple_idx_table",
        "//compile/proxy:vector",
        "//compile/proxy/control_flow:if",
        "//compile/proxy/control_flow:loop",
        "//compile/proxy/value",
        "//execution:pipeline",
        "//khir:program_builder",
        "//plan:skinner_join_operator",
        "//util:vector_util",
        "@absl//absl/container:btree",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "group_by_aggregate_translator",
    srcs = ["group_by_aggregate_translator.cc"],
    hdrs = ["group_by_aggregate_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile/proxy:hash_table",
        "//compile/proxy:struct",
        "//compile/proxy/control_flow:if",
        "//compile/proxy/control_flow:loop",
        "//compile/proxy/value",
        "//execution:pipeline",
        "//khir:program_builder",
        "//plan:group_by_aggregate_operator",
        "//plan:hash_join_operator",
        "//util:vector_util",
    ],
)

cc_library(
    name = "order_by_translator",
    srcs = ["order_by_translator.cc"],
    hdrs = ["order_by_translator.h"],
    deps = [
        ":expression_translator",
        ":operator_translator",
        "//compile/proxy:function",
        "//compile/proxy:vector",
        "//compile/proxy/control_flow:if",
        "//compile/proxy/control_flow:loop",
        "//compile/proxy/value",
        "//execution:pipeline",
        "//khir:program_builder",
        "//plan:order_by_operator",
        "//util:vector_util",
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
        ":permutable_skinner_join_translator",
        ":recompiling_skinner_join_translator",
        ":scan_translator",
        ":select_translator",
        "//compile/proxy:hash_table",
        "//compile/proxy:vector",
        "//execution:pipeline",
        "//khir:program_builder",
        "//plan:hash_join_operator",
        "//plan:operator",
        "//plan:output_operator",
        "//plan:scan_operator",
        "//plan:select_operator",
        "//plan:skinner_join_operator",
        "@absl//absl/flags:flag",
    ],
)
