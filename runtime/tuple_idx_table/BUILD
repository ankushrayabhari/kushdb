load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "key",
    srcs = ["key.cc"],
    hdrs = ["key.h"],
    deps = [],
)

cc_library(
    name = "node",
    srcs = [
        "node.cc",
        "node16.cc",
        "node256.cc",
        "node4.cc",
        "node48.cc",
    ],
    hdrs = [
        "node.h",
        "node16.h",
        "node256.h",
        "node4.h",
        "node48.h",
    ],
    deps = [
        ":key",
    ],
)

cc_library(
    name = "leaf",
    srcs = ["leaf.cc"],
    hdrs = ["leaf.h"],
    deps = [
        ":key",
        ":node",
    ],
)

cc_library(
    name = "tuple_idx_table",
    srcs = ["tuple_idx_table.cc"],
    hdrs = ["tuple_idx_table.h"],
    deps = [
        ":key",
        ":leaf",
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
