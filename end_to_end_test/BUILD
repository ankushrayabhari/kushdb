load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "execute_capture",
    hdrs = ["execute_capture.h"],
    deps = [
        "//compile:query_translator",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.h"],
    deps = [
        "//catalog",
        "//catalog:sql_type",
    ],
)

filegroup(
    name = "data",
    srcs = glob([
        "data/*.kdb",
    ]),
)

filegroup(
    name = "results",
    srcs = glob([
        "results/*.csv",
    ]),
)

cc_test(
    name = "hash_join_test",
    size = "small",
    srcs = ["hash_join_test.cc"],
    data = [
        ":data",
        ":results",
    ],
    deps = [
        ":execute_capture",
        ":schema",
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
        "//plan/expression:column_ref_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "//util:builder",
        "//util:time_execute",
        "//util:vector_util",
        "@absl//absl/flags:parse",
        "@absl//absl/flags:usage",
        "@com_google_googletest//:gtest_main",
    ],
)
