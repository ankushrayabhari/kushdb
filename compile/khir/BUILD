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
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:inlined_vector",
        "@absl//absl/types:span",
        "@type_safe",
    ],
)
