load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "value",
    hdrs = ["value.h"],
    deps = [
        ":printer_h",
        "//compile:program_builder",
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "printer_h",
    hdrs = ["printer.h"],
    deps = [
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "printer_impl",
    srcs = ["printer.cc"],
    deps = [
        ":bool",
        ":float",
        ":int",
        ":printer_h",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "struct",
    srcs = ["struct.cc"],
    hdrs = ["struct.h"],
    deps = [
        ":bool",
        ":float",
        ":int",
        ":value",
        "//catalog:sql_type",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "hash_table",
    srcs = ["hash_table.cc"],
    hdrs = ["hash_table.h"],
    deps = [
        ":struct",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "vector",
    srcs = ["vector.cc"],
    hdrs = ["vector.h"],
    deps = [
        ":if",
        ":int",
        ":ptr",
        ":struct",
        "//catalog:sql_type",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "function",
    srcs = ["function.cc"],
    hdrs = ["function.h"],
    deps = [
        ":bool",
        ":struct",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "ptr",
    hdrs = ["ptr.h"],
    deps = [
        ":value",
        "//compile:program_builder",
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "column_data",
    srcs = ["column_data.cc"],
    hdrs = ["column_data.h"],
    deps = [
        ":bool",
        ":float",
        ":int",
        ":ptr",
        ":value",
        "//catalog:sql_type",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "numeric",
    hdrs = ["numeric.h"],
    deps = [
        ":value",
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "bool",
    srcs = ["bool.cc"],
    hdrs = ["bool.h"],
    deps = [
        ":printer_h",
        ":value",
        "//compile:ir_registry",
        "//compile:program_builder",
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "float",
    srcs = ["float.cc"],
    hdrs = ["float.h"],
    deps = [
        ":bool",
        ":numeric",
        ":printer_h",
        ":value",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "if",
    srcs = ["if.cc"],
    hdrs = ["if.h"],
    deps = [
        ":bool",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "int",
    srcs = ["int.cc"],
    hdrs = ["int.h"],
    deps = [
        ":bool",
        ":numeric",
        ":printer_h",
        ":value",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "loop",
    srcs = ["loop.cc"],
    hdrs = ["loop.h"],
    deps = [
        ":bool",
        ":int",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)
