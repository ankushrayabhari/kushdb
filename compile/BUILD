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
    name = "program",
    hdrs = ["program.h"],
)

cc_library(
    name = "cpp_program",
    srcs = ["cpp_program.cc"],
    hdrs = ["cpp_program.h"],
    linkopts = ["-ldl"],
    deps = [
        ":program",
    ],
)

cc_library(
    name = "compilation_context",
    srcs = ["compilation_context.cc"],
    hdrs = ["compilation_context.h"],
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

cc_library(
    name = "cpp_translator",
    srcs = ["cpp_translator.cc"],
    hdrs = ["cpp_translator.h"],
    deps = [
        ":compilation_context",
        ":cpp_program",
        ":program",
        "//catalog",
        "//compile/translators:translator_factory",
        "//plan:operator",
    ],
)
