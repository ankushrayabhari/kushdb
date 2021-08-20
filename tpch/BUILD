load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

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
        ":data",
    ],
    deps = [
        "//runtime:column_data_builder",
        "@absl//absl/time",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.h"],
    deps = [
        "//catalog",
        "//catalog:sql_type",
    ],
)
