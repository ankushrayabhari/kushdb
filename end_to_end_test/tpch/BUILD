load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "q01_test",
    size = "small",
    srcs = ["q01_test.cc"],
    data = [
        "q01_expected.tbl",
    ],
    deps = [
        "//catalog",
        "//compile:query_translator",
        "//end_to_end_test:parameters",
        "//end_to_end_test:schema",
        "//end_to_end_test:test_util",
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
        "@com_google_googletest//:gtest_main",
    ],
)
