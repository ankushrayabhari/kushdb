load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "if",
    srcs = ["if.cc"],
    hdrs = ["if.h"],
    deps = [
        "//compile/llvm:llvm_program",
        "//compile/proxy:boolean",
    ],
)
