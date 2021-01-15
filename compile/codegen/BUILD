load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "if",
    hdrs = ["if.h"],
    deps = [
        "//compile:program",
        "//compile/proxy:boolean",
    ],
)

cc_library(
    name = "loop",
    hdrs = ["loop.h"],
    deps = [
        "//compile:program",
        "//compile/proxy:boolean",
    ],
)
