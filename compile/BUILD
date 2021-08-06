load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "program",
    hdrs = ["program.h"],
    deps = [],
)

cc_library(
    name = "query_translator",
    srcs = ["query_translator.cc"],
    hdrs = ["query_translator.h"],
    deps = [
        "//catalog",
        "//compile:program",
        "//compile/proxy:column_data",
        "//compile/proxy:column_index",
        "//compile/proxy:hash_table",
        "//compile/proxy:printer_h",
        "//compile/proxy:skinner_join_executor",
        "//compile/proxy:string",
        "//compile/proxy:tuple_idx_table",
        "//compile/proxy:vector",
        "//compile/translators:translator_factory",
        "//khir:program_builder",
        "//khir/asm:asm_backend",
        "//khir/llvm:llvm_backend",
        "@absl//absl/flags:flag",
    ],
)
