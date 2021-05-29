load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "asm_backend",
    srcs = ["asm_backend.cc"],
    hdrs = ["asm_backend.h"],
    deps = [
        "//compile/khir:program_builder",
        "@absl//absl/types:span",
    ],
)
