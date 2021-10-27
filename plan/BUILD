load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "planner",
    srcs = ["planner.cc"],
    hdrs = ["planner.h"],
    deps = [
        "//catalog:catalog_manager",
        "//parse/statement:select_statement",
        "//plan/expression",
        "//plan/operator",
        "//plan/operator:cross_product_operator",
        "//plan/operator:scan_operator",
    ],
)
