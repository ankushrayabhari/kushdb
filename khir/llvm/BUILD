load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "perf_jit_event_listener",
    srcs = ["perf_jit_event_listener.cc"],
    hdrs = ["perf_jit_event_listener.h"],
    deps = [
        "@llvm-project//llvm:BitWriter",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:ExecutionEngine",
        "@llvm-project//llvm:InstCombine",
        "@llvm-project//llvm:OrcJIT",
        "@llvm-project//llvm:Scalar",
        "@llvm-project//llvm:Support",
        "@llvm-project//llvm:X86AsmParser",
        "@llvm-project//llvm:X86CodeGen",
    ],
)

cc_library(
    name = "llvm_backend",
    srcs = ["llvm_backend.cc"],
    hdrs = ["llvm_backend.h"],
    deps = [
        ":perf_jit_event_listener",
        "//khir:program",
        "//khir:program_builder",
        "@absl//absl/flags:flag",
        "@absl//absl/types:span",
        "@llvm-project//llvm:BitWriter",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:ExecutionEngine",
        "@llvm-project//llvm:InstCombine",
        "@llvm-project//llvm:OrcJIT",
        "@llvm-project//llvm:Scalar",
        "@llvm-project//llvm:Support",
        "@llvm-project//llvm:X86AsmParser",
        "@llvm-project//llvm:X86CodeGen",
    ],
)
