load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "expression",
    srcs = ["expression.cc"],
    hdrs = ["expression.h"],
    deps = [
        "@magic_enum//:magic_enum",
        "//util:print_util",
    ],
)


cc_library(
    name = "operator",
    srcs = ["operator.cc"],
    hdrs = ["operator.h"],
    deps = [
        ":expression",
        "//util:print_util",
    ],
)
