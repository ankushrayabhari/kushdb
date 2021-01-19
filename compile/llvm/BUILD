load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "llvm_ir",
    srcs = ["llvm_ir.cc"],
    hdrs = ["llvm_ir.h"],
    deps = [
        "//compile:program",
        "//compile:program_builder",
        "@llvm-project//llvm:BitWriter",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:Support",
    ],
)
