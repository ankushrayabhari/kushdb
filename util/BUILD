load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hash_util",
    hdrs = ["hash_util.h"],
)

cc_library(
    name = "vector_util",
    hdrs = ["vector_util.h"],
)

cc_library(
    name = "visitor",
    hdrs = ["visitor.h"],
)

cc_library(
    name = "print_util",
    srcs = ["print_util.cpp"],
    deps = [
        "//data:string",
    ],
)
