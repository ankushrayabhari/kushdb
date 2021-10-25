load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "data",
    srcs = glob(
        [
            "data/*.kdb",
            "data/*.kdbindex",
        ],
        allow_empty = True,
    ),
)

filegroup(
    name = "raw",
    srcs = glob(
        [
            "raw/*.tbl",
        ],
        allow_empty = True,
    ),
)

cc_binary(
    name = "load",
    srcs = ["load.cc"],
    deps = [
        "//util:load",
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
