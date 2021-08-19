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
    name = "date_extractor",
    srcs = ["date_extractor.cc"],
    hdrs = ["date_extractor.h"],
    linkstatic = True,
    deps = [
        "@absl//absl/time",
    ],
)

cc_binary(
    name = "libdate_extractor.so",
    srcs = ["date_extractor.cc"],
    linkshared = True,
    deps = [
        ":date_extractor",
        "@absl//absl/time",
    ],
)

cc_library(
    name = "column_index",
    srcs = ["column_index.cc"],
    hdrs = ["column_index.h"],
    linkstatic = True,
    deps = [
        ":string",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_binary(
    name = "libcolumn_index.so",
    srcs = ["column_index.cc"],
    linkshared = True,
    deps = [
        ":column_index",
        ":string",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "column_data",
    srcs = ["column_data.cc"],
    hdrs = ["column_data.h"],
    linkstatic = True,
    deps = [
        ":string",
    ],
)

cc_binary(
    name = "libcolumn_data.so",
    srcs = ["column_data.cc"],
    linkshared = True,
    deps = [
        ":column_data",
        ":string",
    ],
)

cc_library(
    name = "string",
    srcs = ["string.cc"],
    hdrs = ["string.h"],
    linkstatic = True,
    deps = [
        "@re2",
    ],
)

cc_binary(
    name = "libstring.so",
    srcs = ["string.cc"],
    linkshared = True,
    deps = [
        ":string",
        "@re2",
    ],
)

cc_library(
    name = "vector",
    srcs = ["vector.cc"],
    hdrs = ["vector.h"],
    linkstatic = True,
    deps = [],
)

cc_binary(
    name = "libvector.so",
    srcs = ["vector.cc"],
    linkshared = True,
    deps = [
        ":vector",
    ],
)

cc_library(
    name = "hash_table",
    srcs = ["hash_table.cc"],
    hdrs = ["hash_table.h"],
    linkstatic = True,
    deps = [
        ":vector",
    ],
)

cc_binary(
    name = "libhash_table.so",
    srcs = ["hash_table.cc"],
    linkshared = True,
    deps = [
        ":hash_table",
        ":vector",
    ],
)

cc_library(
    name = "tuple_idx_table",
    srcs = ["tuple_idx_table.cc"],
    hdrs = ["tuple_idx_table.h"],
    linkstatic = True,
    deps = [],
)

cc_binary(
    name = "libtuple_idx_table.so",
    srcs = ["tuple_idx_table.cc"],
    linkshared = True,
    deps = [
        ":tuple_idx_table",
    ],
)

cc_library(
    name = "skinner_join_executor",
    srcs = ["skinner_join_executor.cc"],
    hdrs = ["skinner_join_executor.h"],
    linkstatic = True,
    deps = [
        "//compile/translators:recompiling_join_translator",
        "@absl//absl/container:btree",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_binary(
    name = "libskinner_join_executor.so",
    srcs = ["skinner_join_executor.cc"],
    linkshared = True,
    deps = [
        ":skinner_join_executor",
        "//compile/translators:recompiling_join_translator",
        "@absl//absl/container:btree",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "column_data_builder",
    hdrs = ["column_data_builder.h"],
    deps = [
        ":column_data",
    ],
)

cc_library(
    name = "printer",
    srcs = ["printer.cc"],
    hdrs = ["printer.h"],
    linkstatic = True,
    deps = [
        ":string",
    ],
)

cc_binary(
    name = "libprinter.so",
    srcs = ["printer.cc"],
    linkshared = True,
    deps = [
        ":printer",
        ":string",
    ],
)
