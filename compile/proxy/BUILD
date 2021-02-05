load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "type",
    hdrs = ["type.h"],
)

cc_library(
    name = "value",
    hdrs = ["value.h"],
    deps = [
        "//compile:program_builder",
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "vector",
    hdrs = ["vector.h"],
    deps = [
        ":int",
        ":ptr",
        "//catalog:sql_type",
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
        ":type",
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
        ":type",
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
        ":type",
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
        ":if",
        ":int",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)
