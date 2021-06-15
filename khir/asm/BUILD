load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "live_intervals",
    srcs = ["live_intervals.cc"],
    hdrs = ["live_intervals.h"],
    deps = [
        "//khir:program_builder",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "register",
    srcs = ["register.cc"],
    hdrs = ["register.h"],
    deps = [
        "@asmjit",
    ],
)

cc_library(
    name = "asm_backend",
    srcs = ["asm_backend.cc"],
    hdrs = ["asm_backend.h"],
    deps = [
        ":live_intervals",
        "//compile:program",
        "//khir:opcode",
        "//khir:program_builder",
        "//khir:program_printer",
        "//khir:type_manager",
        "@absl//absl/types:span",
        "@asmjit",
    ],
)
