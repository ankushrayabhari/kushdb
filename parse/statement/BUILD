load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "statement",
    hdrs = ["statement.h"],
    deps = [],
)

cc_library(
    name = "select_statement",
    hdrs = ["select_statement.h"],
    deps = [
        "//parse/expression",
    ],
)
