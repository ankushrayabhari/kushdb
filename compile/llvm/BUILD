load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "llvm_ir",
    srcs = ["llvm_ir.cc"],
    hdrs = ["llvm_ir.h"],
    linkopts = ["-ldl"],
    deps = [
        "//compile:program",
        "//compile:program_builder",
        "//data:column_data",
        "//data:hash_table",
        "//data:vector",
        "//util:print_util",
        "@llvm-project//llvm:BitWriter",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:InstCombine",
        "@llvm-project//llvm:Scalar",
        "@llvm-project//llvm:Support",
        "@llvm-project//llvm:X86AsmParser",
        "@llvm-project//llvm:X86CodeGen",
    ],
)
