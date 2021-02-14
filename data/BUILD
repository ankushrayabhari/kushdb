load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "column_data",
    srcs = ["column_data.cc"],
    hdrs = ["column_data.h"],
    deps = [],
)

cc_library(
    name = "column_data_builder",
    hdrs = ["column_data_builder.h"],
    deps = [
        ":column_data",
    ],
)
