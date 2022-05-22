load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "executable_query",
    srcs = ["executable_query.cc"],
    hdrs = ["executable_query.h"],
    deps = [
        ":pipeline",
        ":query_state",
        "//compile/translators:operator_translator",
        "//khir:backend",
        "//khir/asm:asm_backend",
        "//khir/asm:reg_alloc_impl",
        "//khir/llvm:llvm_backend",
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

cc_library(
    name = "query_state",
    hdrs = ["query_state.h"],
    deps = [],
)
