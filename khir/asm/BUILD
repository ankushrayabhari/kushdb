load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "register_assignment",
    srcs = ["register_assignment.cc"],
    hdrs = ["register_assignment.h"],
    deps = [],
)

cc_library(
    name = "stack_spill_reg_alloc",
    srcs = ["stack_spill_reg_alloc.cc"],
    hdrs = ["stack_spill_reg_alloc.h"],
    deps = [
        ":register_assignment",
        "//khir:instruction",
        "//khir:opcode",
    ],
)

cc_library(
    name = "linear_scan_reg_alloc",
    srcs = ["linear_scan_reg_alloc.cc"],
    hdrs = ["linear_scan_reg_alloc.h"],
    deps = [
        ":live_intervals",
        ":register",
        ":register_assignment",
        "//khir:instruction",
        "//khir:opcode",
    ],
)

cc_library(
    name = "live_intervals",
    srcs = ["live_intervals.cc"],
    hdrs = ["live_intervals.h"],
    deps = [
        "//khir:program_builder",
        "//util:union_find",
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
        ":linear_scan_reg_alloc",
        ":live_intervals",
        ":register_assignment",
        ":stack_spill_reg_alloc",
        "//compile:program",
        "//khir:opcode",
        "//khir:program_builder",
        "//khir:program_printer",
        "//khir:type_manager",
        "@absl//absl/types:span",
        "@asmjit",
    ],
)

cc_test(
    name = "asm_backend_test",
    size = "small",
    srcs = ["asm_backend_test.cc"],
    deps = [
        ":asm_backend",
        "//khir:program_builder",
        "@com_google_googletest//:gtest_main",
    ],
)
