load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "value",
    hdrs = ["value.h"],
    deps = [
        "//compile:program_builder",
    ],
)

cc_library(
    name = "bool",
    srcs = ["bool.cc"],
    hdrs = ["bool.h"],
    deps = [
        ":value",
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)

cc_library(
    name = "float",
    srcs = ["float.cc"],
    hdrs = ["float.h"],
    deps = [
        ":bool",
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
        "//compile:ir_registry",
        "//compile:program_builder",
    ],
)
