load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "opcode",
    hdrs = ["opcode.h"],
    deps = [],
)

cc_library(
    name = "program_builder",
    srcs = ["program_builder.cc"],
    hdrs = ["program_builder.h"],
    deps = [
        ":instruction",
        ":opcode",
        ":type_manager",
        "//compile:program",
        "@absl//absl/types:span",
        "@type_safe",
    ],
)

cc_library(
    name = "type_manager",
    srcs = ["type_manager.cc"],
    hdrs = ["type_manager.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/types:span",
        "@llvm-project//llvm:BitWriter",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:InstCombine",
        "@llvm-project//llvm:Scalar",
        "@llvm-project//llvm:Support",
        "@llvm-project//llvm:X86AsmParser",
        "@llvm-project//llvm:X86CodeGen",
        "@type_safe",
    ],
)

cc_library(
    name = "instruction",
    srcs = ["instruction.cc"],
    hdrs = ["instruction.h"],
    deps = [
        ":opcode",
    ],
)
