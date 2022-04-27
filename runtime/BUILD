load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "file_manager",
    srcs = ["file_manager.cc"],
    hdrs = ["file_manager.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "date",
    srcs = ["date.cc"],
    hdrs = ["date.h"],
    deps = [],
)

cc_test(
    name = "date_test",
    size = "small",
    srcs = ["date_test.cc"],
    deps = [
        ":date",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "column_index_bucket",
    srcs = ["column_index_bucket.cc"],
    hdrs = ["column_index_bucket.h"],
    deps = [],
)

cc_library(
    name = "memory_column_index",
    srcs = ["memory_column_index.cc"],
    hdrs = ["memory_column_index.h"],
    deps = [
        ":column_index_bucket",
        ":string",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name = "column_index_bucket_test",
    size = "small",
    srcs = ["column_index_bucket_test.cc"],
    deps = [
        ":column_index_bucket",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "column_index",
    srcs = ["column_index.cc"],
    hdrs = ["column_index.h"],
    deps = [
        ":column_index_bucket",
        ":file_manager",
        ":string",
    ],
)

cc_library(
    name = "column_data",
    srcs = ["column_data.cc"],
    hdrs = ["column_data.h"],
    deps = [
        ":date",
        ":file_manager",
        ":string",
    ],
)

cc_library(
    name = "string",
    srcs = ["string.cc"],
    hdrs = ["string.h"],
    deps = [
        "@re2",
    ],
)

cc_library(
    name = "allocator",
    srcs = ["allocator.cc"],
    hdrs = ["allocator.h"],
    deps = [],
)

cc_library(
    name = "vector",
    srcs = ["vector.cc"],
    hdrs = ["vector.h"],
    deps = [],
)

cc_library(
    name = "hash_table",
    srcs = ["hash_table.cc"],
    hdrs = ["hash_table.h"],
    deps = [
        ":vector",
    ],
)

cc_library(
    name = "aggregate_hash_table",
    srcs = ["aggregate_hash_table.cc"],
    hdrs = ["aggregate_hash_table.h"],
    deps = [],
)

cc_library(
    name = "skinner_join_executor",
    srcs = ["skinner_join_executor.cc"],
    hdrs = ["skinner_join_executor.h"],
    deps = [
        "//compile/translators:recompiling_join_translator",
        "@absl//absl/container:btree",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/flags:flag",
    ],
)

cc_library(
    name = "skinner_scan_select_executor",
    srcs = ["skinner_scan_select_executor.cc"],
    hdrs = ["skinner_scan_select_executor.h"],
    deps = [
        "@absl//absl/container:btree",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/flags:flag",
    ],
)

cc_library(
    name = "printer",
    srcs = ["printer.cc"],
    hdrs = ["printer.h"],
    deps = [
        ":date",
        ":string",
    ],
)
