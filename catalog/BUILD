load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "catalog",
    srcs = ["catalog.cc"],
    hdrs = ["catalog.h"],
    deps = [
        "//data:column_data",
    ],
)
