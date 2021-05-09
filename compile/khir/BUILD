load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "opcode",
    hdrs = ["opcode.h"],
    deps = [],
)

cc_library(
    name = "khir_program",
    srcs = ["khir_program.cc"],
    hdrs = ["khir_program.h"],
    deps = [
        ":instruction",
        ":opcode",
        ":type_manager",
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
