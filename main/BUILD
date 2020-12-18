load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_binary(
    name = "main",
    srcs = ["main.cc"],
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
        "//util:vector_util",
        "@json",
    ],
)

cc_binary(
    name = "generate_vector",
    srcs = ["generate_vector.cc"],
    deps = [
        "//data:column_data",
    ],
)

cc_binary(
    name = "read_vector",
    srcs = ["read_vector.cc"],
    deps = [
        "//data:column_data",
    ],
)

cc_test(
    name = "main_test",
    srcs = ["main_test.cc"],
    deps = [
        "@googletest//:gtest_main",
    ],
)
