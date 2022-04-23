load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "libpgquery_base",
    srcs = glob([
        "*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.hpp",
    ]),
    includes = [
        "include",
    ],
    deps = [],
)

cc_library(
    name = "libpgquery",
    hdrs = ["parser.h"],
    strip_include_prefix = "/third_party/",
    visibility = ["//visibility:public"],
    deps = [":libpgquery_base"],
)
