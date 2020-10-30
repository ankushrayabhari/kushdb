load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "operator",
    srcs = ["operator.cc"],
    hdrs = ["operator.h"],
    deps = [
        "//plan/expression",
        "//plan/expression:column_ref_expression",
        "@json",
    ],
)
