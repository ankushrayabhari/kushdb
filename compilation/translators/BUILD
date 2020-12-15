load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "translator",
    srcs = ["translator.cc"],
    hdrs = ["translator.h"],
)

cc_library(
    name = "scan_translator",
    srcs = ["scan_translator.cc"],
    hdrs = ["scan_translator.h"],
    deps = [
        ":translator",
        "//compilation:cpp_translator",
        "//plan:operator",
    ],
)

cc_library(
    name = "output_translator",
    hdrs = ["output_translator.h"],
    deps = [
        ":translator",
        "//compilation:cpp_translator",
        "//plan:operator",
    ],
)

cc_library(
    name = "select_translator",
    hdrs = ["select_translator.h"],
    deps = [
        ":translator",
        "//compilation:cpp_translator",
        "//plan:operator",
    ],
)

cc_library(
    name = "hash_join_translator",
    hdrs = ["hash_join_translator.h"],
    deps = [
        ":translator",
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
        ":output_translator",
        ":scan_translator",
        ":select_translator",
        ":translator",
    ],
)
