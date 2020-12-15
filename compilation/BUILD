load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "program",
    hdrs = ["program.h"],
)

cc_library(
    name = "cpp_program",
    srcs = ["cpp_program.cc"],
    hdrs = ["cpp_program.h"],
    deps = [
        ":program",
    ],
)

cc_library(
    name = "compilation_context",
    srcs = ["compilation_context.cc"],
    hdrs = ["compilation_context.h"],
    linkopts = ["-ldl"],
    deps = [
        ":cpp_program",
        "//catalog",
        "//plan:operator",
        "//plan/expression",
    ],
)

cc_library(
    name = "schema_values",
    srcs = ["schema_values.cc"],
    hdrs = ["schema_values.h"],
)
