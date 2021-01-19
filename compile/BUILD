load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "program",
    hdrs = ["program.h"],
    deps = [],
)

cc_library(
    name = "program_registry",
    hdrs = ["program_registry.h"],
    deps = [
        "//compile/llvm:llvm_program",
    ],
)

cc_library(
    name = "program_builder",
    hdrs = ["program_builder.h"],
    deps = [],
)

cc_library(
    name = "query_translator",
    srcs = ["query_translator.cc"],
    hdrs = ["query_translator.h"],
    deps = [
        "//catalog",
        "//compile:program",
        "//compile/translators:translator_factory",
    ],
)
