load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "parser",
    srcs = ["parser.cc"],
    hdrs = ["parser.h"],
    deps = [
        "//parse/statement",
        "//parse/transform",
        "//third_party/duckdb_libpgquery",
    ],
)
