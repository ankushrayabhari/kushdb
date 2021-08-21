load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "raw",
    srcs = glob(
        [
            "raw/*.tbl",
        ],
        allow_empty = True,
    ),
)

filegroup(
    name = "data",
    srcs = glob(
        [
            "data/*.kdb",
        ],
        allow_empty = True,
    ),
)

cc_binary(
    name = "load",
    srcs = ["load.cc"],
    data = [
        ":raw",
    ],
    deps = [
        "//runtime:column_data_builder",
        "@absl//absl/time",
    ],
)

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
    data = [
        ":data",
    ],
    deps = [
        "//catalog",
        "//catalog:sql_type",
    ],
)
