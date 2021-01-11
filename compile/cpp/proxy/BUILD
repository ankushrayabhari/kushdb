load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "value",
    hdrs = ["value.h"],
    deps = [
        "//plan/expression:binary_arithmetic_expression",
    ],
)

cc_library(
    name = "boolean",
    srcs = ["boolean.cc"],
    hdrs = ["boolean.h"],
    deps = [
        ":value",
        "//compile/cpp:cpp_program",
    ],
)

cc_library(
    name = "integer",
    srcs = ["integer.cc"],
    hdrs = ["integer.h"],
    deps = [
        ":boolean",
        ":value",
        "//compile/cpp:cpp_program",
    ],
)

cc_library(
    name = "double",
    srcs = ["double.cc"],
    hdrs = ["double.h"],
    deps = [
        ":boolean",
        ":value",
        "//compile/cpp:cpp_program",
    ],
)

cc_library(
    name = "string_view",
    srcs = ["string_view.cc"],
    hdrs = ["string_view.h"],
    deps = [
        ":boolean",
        ":value",
        "//compile/cpp:cpp_program",
    ],
)
