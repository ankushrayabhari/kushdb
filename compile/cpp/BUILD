load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "types",
    srcs = ["types.cc"],
    hdrs = ["types.h"],
    deps = [
        "//catalog:sql_type",
    ],
)

cc_library(
    name = "cpp_program",
    srcs = ["cpp_program.cc"],
    hdrs = ["cpp_program.h"],
    linkopts = ["-ldl"],
    deps = [
        "//compile:program",
    ],
)

cc_library(
    name = "cpp_compilation_context",
    srcs = ["cpp_compilation_context.cc"],
    hdrs = ["cpp_compilation_context.h"],
    deps = [
        ":cpp_program",
        "//catalog",
        "//plan:operator",
        "//plan/expression",
    ],
)

cc_library(
    name = "cpp_translator",
    srcs = ["cpp_translator.cc"],
    hdrs = ["cpp_translator.h"],
    deps = [
        ":cpp_compilation_context",
        ":cpp_program",
        "//catalog",
        "//compile:program",
        "//compile/cpp/translators:translator_factory",
        "//plan:operator",
    ],
)
