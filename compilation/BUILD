load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "translator_registry",
    srcs = ["translator_registry.cc"],
    hdrs = ["translator_registry.h"],
    deps = [
        "//algebra:operator",
    ],
)

cc_library(
    name = "translator",
    hdrs = ["translator.h"],
    deps = [
        ":translator_registry",
        "//algebra:operator",
    ],
)

cc_library(
    name = "cpp_translator",
    srcs = ["cpp_translator.cc"],
    hdrs = ["cpp_translator.h"],
    deps = [
        ":translator",
        ":translator_registry",
        "//algebra:operator",
    ],
)
