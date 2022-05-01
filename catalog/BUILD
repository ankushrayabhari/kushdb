load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "catalog_manager",
    srcs = ["catalog_manager.cc"],
    hdrs = ["catalog_manager.h"],
    deps = [
        ":catalog",
    ],
)

cc_library(
    name = "catalog",
    srcs = ["catalog.cc"],
    hdrs = ["catalog.h"],
    deps = [
        ":sql_type",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "sql_type",
    hdrs = ["sql_type.h"],
    deps = [
        "@magic_enum",
    ],
)
