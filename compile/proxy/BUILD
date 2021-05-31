load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "value",
    hdrs = ["value.h"],
    deps = [
        ":printer_h",
        "//khir:program_builder",
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "printer_h",
    hdrs = ["printer.h"],
    deps = [
        "//khir:program_builder",
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
        ":string",
        "//khir:program_builder",
        "//runtime:printer",
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
        ":string",
        ":value",
        "//catalog:sql_type",
        "//khir:program_builder",
    ],
)

cc_library(
    name = "hash_table",
    srcs = ["hash_table.cc"],
    hdrs = ["hash_table.h"],
    deps = [
        ":loop",
        ":struct",
        ":vector",
        "//khir:program_builder",
        "//runtime:hash_table",
        "//util:vector_util",
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
        "//khir:program_builder",
        "//runtime:vector",
    ],
)

cc_library(
    name = "tuple_idx_table",
    srcs = ["tuple_idx_table.cc"],
    hdrs = ["tuple_idx_table.h"],
    deps = [
        ":int",
        ":loop",
        "//khir:program_builder",
        "//runtime:tuple_idx_table",
        "//util:vector_util",
    ],
)

cc_library(
    name = "function",
    srcs = ["function.cc"],
    hdrs = ["function.h"],
    deps = [
        ":bool",
        ":struct",
        "//khir:program_builder",
    ],
)

cc_library(
    name = "skinner_join_executor",
    srcs = ["skinner_join_executor.cc"],
    hdrs = ["skinner_join_executor.h"],
    deps = [
        ":int",
        "//khir:program_builder",
        "//runtime:skinner_join_executor",
    ],
)

cc_library(
    name = "ptr",
    hdrs = ["ptr.h"],
    deps = [
        ":value",
        "//khir:program_builder",
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
        ":string",
        ":value",
        "//catalog:sql_type",
        "//khir:program_builder",
        "//runtime:column_data",
    ],
)

cc_library(
    name = "column_index",
    srcs = ["column_index.cc"],
    hdrs = ["column_index.h"],
    deps = [
        ":bool",
        ":float",
        ":int",
        ":ptr",
        ":string",
        ":value",
        "//catalog:sql_type",
        "//khir:program_builder",
        "//runtime:column_index",
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
    name = "string",
    srcs = ["string.cc"],
    hdrs = ["string.h"],
    deps = [
        ":bool",
        ":printer_h",
        ":value",
        "//khir:program_builder",
        "//plan/expression:binary_arithmetic_expression",
        "//runtime:string",
    ],
)

cc_library(
    name = "bool",
    srcs = ["bool.cc"],
    hdrs = ["bool.h"],
    deps = [
        ":printer_h",
        ":value",
        "//khir:program_builder",
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "float",
    srcs = ["float.cc"],
    hdrs = ["float.h"],
    deps = [
        ":bool",
        ":int",
        ":numeric",
        ":printer_h",
        ":value",
        "//khir:program_builder",
    ],
)

cc_library(
    name = "if",
    srcs = ["if.cc"],
    hdrs = ["if.h"],
    deps = [
        ":bool",
        "//khir:program_builder",
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
        "//khir:program_builder",
    ],
)

cc_library(
    name = "loop",
    srcs = ["loop.cc"],
    hdrs = ["loop.h"],
    deps = [
        ":bool",
        ":int",
        "//khir:program_builder",
    ],
)
