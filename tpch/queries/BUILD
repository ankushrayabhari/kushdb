load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "q1",
    srcs = ["q01.cc"],
    deps = [
        "//catalog",
        "//compile:cpp_translator",
        "//plan:group_by_aggregate_operator",
        "//plan:hash_join_operator",
        "//plan:operator",
        "//plan:output_operator",
        "//plan:scan_operator",
        "//plan:select_operator",
        "//plan/expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:comparison_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "//tpch:schema",
        "//util:vector_util",
    ],
)

cc_binary(
    name = "q3",
    srcs = ["q03.cc"],
    deps = [
        "//catalog",
        "//compile:cpp_translator",
        "//plan:group_by_aggregate_operator",
        "//plan:hash_join_operator",
        "//plan:operator",
        "//plan:output_operator",
        "//plan:scan_operator",
        "//plan:select_operator",
        "//plan/expression",
        "//plan/expression:column_ref_expression",
        "//plan/expression:comparison_expression",
        "//plan/expression:literal_expression",
        "//plan/expression:virtual_column_ref_expression",
        "//tpch:schema",
        "//util:vector_util",
    ],
)
