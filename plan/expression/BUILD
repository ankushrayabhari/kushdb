load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "expression",
    srcs = ["expression.cc"],
    hdrs = ["expression.h"],
    deps = [
        "//util:print_util",
        "@magic_enum",
    ],
)

cc_library(
    name = "literal_expression",
    srcs = ["literal_expression.cc"],
    hdrs = ["literal_expression.h"],
)
