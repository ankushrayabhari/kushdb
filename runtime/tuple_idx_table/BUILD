load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "node",
    srcs = [
        "allocator.cc",
        "key.cc",
        "leaf.cc",
        "node.cc",
        "node16.cc",
        "node256.cc",
        "node4.cc",
        "node48.cc",
    ],
    hdrs = [
        "allocator.h",
        "key.h",
        "leaf.h",
        "node.h",
        "node16.h",
        "node256.h",
        "node4.h",
        "node48.h",
    ],
    deps = [],
)

cc_library(
    name = "tuple_idx_table",
    srcs = ["tuple_idx_table.cc"],
    hdrs = ["tuple_idx_table.h"],
    deps = [
        ":node",
    ],
)

cc_test(
    name = "tuple_idx_table_test",
    size = "small",
    srcs = ["tuple_idx_table_test.cc"],
    deps = [
        ":tuple_idx_table",
        "@com_google_googletest//:gtest_main",
    ],
)
