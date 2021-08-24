load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "llvm_backend",
    srcs = ["llvm_backend.cc"],
    hdrs = ["llvm_backend.h"],
    data = [
        "//runtime:libcolumn_data.so",
        "//runtime:libcolumn_index.so",
        "//runtime:libdate_extractor.so",
        "//runtime:libhash_table.so",
        "//runtime:libprinter.so",
        "//runtime:libskinner_join_executor.so",
        "//runtime:libstring.so",
        "//runtime:libtuple_idx_table.so",
        "//runtime:libvector.so",
    ],
    deps = [
        "//compile:program",
        "//khir:program_builder",
        "@absl//absl/flags:flag",
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
