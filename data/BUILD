load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "column_data_printer",
    srcs = ["column_data_printer.cc"],
    deps = [
        ":column_data",
        ":vector",
    ],
)

cc_library(
    name = "column_data",
    srcs = ["column_data.cc"],
    hdrs = ["column_data.h"],
    deps = [
        ":string",
    ],
)

cc_library(
    name = "string",
    srcs = ["string.cc"],
    hdrs = ["string.h"],
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
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "column_data_builder",
    hdrs = ["column_data_builder.h"],
    deps = [
        ":column_data",
    ],
)
