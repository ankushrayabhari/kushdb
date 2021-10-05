load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "if",
    srcs = ["if.cc"],
    hdrs = ["if.h"],
    deps = [
        "//compile/proxy/value:ir_value",
        "//compile/proxy/value:sql_value",
        "//khir:program_builder",
    ],
)

cc_library(
    name = "loop",
    srcs = ["loop.cc"],
    hdrs = ["loop.h"],
    deps = [
        "//compile/proxy/value:ir_value",
        "//khir:program_builder",
    ],
)
