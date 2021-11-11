load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
    name = "query_runner",
    srcs = ["query_runner.cc"],
    data = [
        ":data",
        ":queries",
    ],
    deps = [
        ":schema",
        "//catalog:catalog_manager",
        "//parse:parser",
        "//plan:planner",
        "//util:time_execute",
        "@absl//absl/flags:parse",
        "@absl//absl/flags:usage",
    ],
)

filegroup(
    name = "queries",
    srcs = glob(
        [
            "queries/*.sql",
        ],
        allow_empty = True,
    ),
)

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

cc_binary(
    name = "load",
    srcs = ["load.cc"],
    deps = [
        "//util:load",
    ],
)
