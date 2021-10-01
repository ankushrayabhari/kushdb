load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "executable_query",
    srcs = ["executable_query.cc"],
    hdrs = ["executable_query.h"],
    deps = [
        ":pipeline",
        "//compile/translators:operator_translator",
        "//khir:program",
    ],
)

cc_library(
    name = "pipeline",
    srcs = ["pipeline.cc"],
    hdrs = ["pipeline.h"],
    deps = [
        "//util:vector_util",
    ],
)
