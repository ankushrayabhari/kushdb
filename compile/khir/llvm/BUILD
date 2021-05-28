load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "khir_llvm_backend",
    srcs = ["khir_llvm_backend.cc"],
    hdrs = ["khir_llvm_backend.h"],
    deps = [
        "//compile/khir:khir_program_builder",
        "@absl//absl/types:span",
        "@llvm-project//llvm:BitWriter",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:InstCombine",
        "@llvm-project//llvm:Scalar",
        "@llvm-project//llvm:Support",
        "@llvm-project//llvm:X86AsmParser",
        "@llvm-project//llvm:X86CodeGen",
    ],
)
