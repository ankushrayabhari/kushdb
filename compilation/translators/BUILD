load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "operator_translator",
    srcs = ["operator_translator.cc"],
    hdrs = ["operator_translator.h"],
)

cc_library(
    name = "scan_translator",
    srcs = ["scan_translator.cc"],
    hdrs = ["scan_translator.h"],
    deps = [
        ":operator_translator",
        "//compilation:cpp_translator",
        "//plan:operator",
    ],
)

cc_library(
    name = "output_translator",
    hdrs = ["output_translator.h"],
    deps = [
        ":operator_translator",
        "//compilation:cpp_translator",
        "//plan:operator",
    ],
)

cc_library(
    name = "select_translator",
    hdrs = ["select_translator.h"],
    deps = [
        ":operator_translator",
        "//compilation:cpp_translator",
        "//plan:operator",
    ],
)

cc_library(
    name = "hash_join_translator",
    hdrs = ["hash_join_translator.h"],
    deps = [
        ":operator_translator",
        "//compilation:cpp_translator",
        "//plan:operator",
    ],
)

cc_library(
    name = "translator_factory",
    srcs = ["translator_factory.cc"],
    hdrs = ["translator_factory.h"],
    deps = [
        ":hash_join_translator",
        ":operator_translator",
        ":output_translator",
        ":scan_translator",
        ":select_translator",
    ],
)

cc_library(
    name = "expression_translator",
    srcs = ["expression_translator.cc"],
    hdrs = ["expression_translator.h"],
    deps = [
        "//compilation:cpp_translator",
        "//plan/expression:expression_visitor",
    ],
)
