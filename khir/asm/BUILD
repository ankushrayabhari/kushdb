load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "reg_alloc_impl",
    srcs = ["reg_alloc_impl.cc"],
    hdrs = ["reg_alloc_impl.h"],
    deps = [
        "@absl//absl/flags:flag",
    ],
)

cc_library(
    name = "loops",
    srcs = ["loops.cc"],
    hdrs = ["loops.h"],
    deps = [
        ":dominators",
        "//util:union_find",
        "@absl//absl/container:btree",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_test(
    name = "loops_test",
    size = "small",
    srcs = ["loops_test.cc"],
    deps = [
        ":dominators",
        ":loops",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "dominators",
    srcs = ["dominators.cc"],
    hdrs = ["dominators.h"],
    deps = [
        "@absl//absl/container:btree",
    ],
)

cc_test(
    name = "dominators_test",
    size = "small",
    srcs = ["dominators_test.cc"],
    deps = [
        ":dominators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "dfs_label",
    srcs = ["dfs_label.cc"],
    hdrs = ["dfs_label.h"],
    deps = [],
)

cc_test(
    name = "dfs_label_test",
    size = "small",
    srcs = ["dfs_label_test.cc"],
    deps = [
        ":dfs_label",
        "@com_google_googletest//:gtest_main",
    ],
)

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
        ":dfs_label",
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
        ":dfs_label",
        ":loops",
        "//khir:program_builder",
        "//util:union_find",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_test(
    name = "live_intervals_test",
    size = "small",
    srcs = ["live_intervals_test.cc"],
    deps = [
        ":live_intervals",
        "//khir:program_builder",
        "//khir:program_printer",
        "@com_google_googletest//:gtest_main",
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
        ":dfs_label",
        ":linear_scan_reg_alloc",
        ":live_intervals",
        ":reg_alloc_impl",
        ":register_assignment",
        ":stack_spill_reg_alloc",
        "//khir:opcode",
        "//khir:program",
        "//khir:program_builder",
        "//khir:program_printer",
        "//khir:type_manager",
        "//util:profile_map_generator",
        "@absl//absl/types:span",
        "@asmjit",
    ],
)
