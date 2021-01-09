load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "if",
    srcs = ["if.cc"],
    hdrs = ["if.h"],
    deps = [
        "//compile/cpp:cpp_program",
        "//compile/cpp/proxy:boolean",
    ],
)
