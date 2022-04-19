load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "value",
    srcs = ["value.cc"],
    hdrs = ["value.h"],
    deps = [],
)

cc_library(
    name = "compilation_cache",
    srcs = ["compilation_cache.cc"],
    hdrs = ["compilation_cache.h"],
    deps = [
        ":backend",
        ":program_builder",
        "//khir/asm:asm_backend",
        "//khir/asm:reg_alloc_impl",
        "//khir/llvm:llvm_backend",
    ],
)

cc_library(
    name = "backend",
    srcs = ["backend.cc"],
    hdrs = ["backend.h"],
    deps = [
        "@absl//absl/flags:flag",
    ],
)

cc_library(
    name = "opcode",
    srcs = ["opcode.cc"],
    hdrs = ["opcode.h"],
    deps = [],
)

cc_library(
    name = "program_printer",
    srcs = ["program_printer.cc"],
    hdrs = ["program_printer.h"],
    deps = [
        ":instruction",
        ":opcode",
        ":program_builder",
        "@magic_enum",
    ],
)

cc_library(
    name = "program_builder",
    srcs = ["program_builder.cc"],
    hdrs = ["program_builder.h"],
    deps = [
        ":instruction",
        ":opcode",
        ":type_manager",
        ":value",
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

cc_test(
    name = "backend_test",
    size = "medium",
    srcs = ["backend_test.cc"],
    deps = [
        ":backend",
        ":program_builder",
        "//khir/asm:asm_backend",
        "//khir/llvm:llvm_backend",
        "@com_google_googletest//:gtest_main",
    ],
)
