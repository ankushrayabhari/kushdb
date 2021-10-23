load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "duckdb_libpgquery_base",
    srcs = glob([
        "*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.hpp",
    ]),
    includes = [
        "include",
    ],
    deps = [],
)

cc_library(
    name = "duckdb_libpgquery",
    hdrs = ["parser.h"],
    strip_include_prefix = "/third_party/",
    visibility = ["//visibility:public"],
    deps = [":duckdb_libpgquery_base"],
)
