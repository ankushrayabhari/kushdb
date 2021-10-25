load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "statement",
    hdrs = ["statement.h"],
    deps = [],
)

cc_library(
    name = "select_statement",
    srcs = ["select_statement.cc"],
    hdrs = ["select_statement.h"],
    deps = [
        ":statement",
        "//parse/expression",
        "//parse/table",
    ],
)
