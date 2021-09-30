load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_translator",
    srcs = ["query_translator.cc"],
    hdrs = ["query_translator.h"],
    deps = [
        ":forward_declare",
        "//compile/translators:operator_translator",
        "//compile/translators:translator_factory",
        "//khir:backend",
        "//khir:program",
        "//khir:program_builder",
        "//khir/asm:asm_backend",
        "//khir/asm:reg_alloc_impl",
        "//khir/llvm:llvm_backend",
    ],
)

cc_library(
    name = "forward_declare",
    srcs = ["forward_declare.cc"],
    hdrs = ["forward_declare.h"],
    deps = [
        "//catalog:sql_type",
        "//compile/proxy:column_data",
        "//compile/proxy:column_index",
        "//compile/proxy:hash_table",
        "//compile/proxy:printer_h",
        "//compile/proxy:skinner_join_executor",
        "//compile/proxy:string",
        "//compile/proxy:tuple_idx_table",
        "//compile/proxy:vector",
        "//compile/translators:expression_translator",
        "//khir:program",
        "//khir:program_builder",
    ],
)
