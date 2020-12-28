load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "llvm_program",
    srcs = ["llvm_program.cc"],
    hdrs = ["llvm_program.h"],
    deps = [
        "//compile:program",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:Support",
    ],
)
