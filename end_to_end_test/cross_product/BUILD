load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "cross_product_test",
    size = "small",
    srcs = ["cross_product_test.cc"],
    data = [
        "cross_product_expected.tbl",
    ],
    deps = [
        "//catalog",
        "//compile:query_translator",
        "//end_to_end_test:parameters",
        "//end_to_end_test:schema",
        "//plan/expression",
        "//plan/expression:arithmetic_expression",
        "//plan/expression:column_ref_expression",
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
        "//util:builder",
        "//util:test_util",
        "//util:time_execute",
        "//util:vector_util",
        "@com_google_googletest//:gtest_main",
    ],
)
