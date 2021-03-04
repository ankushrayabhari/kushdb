load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "load",
    srcs = ["load.cc"],
    linkopts = [
        "-pthread",
        "-ltbb",
    ],
    deps = [
        "//data:column_data_builder",
        "@absl//absl/time",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.h"],
    deps = [
        "//catalog",
        "//catalog:sql_type",
    ],
)
