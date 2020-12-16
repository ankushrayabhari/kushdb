load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "catalog",
    srcs = ["catalog.cc"],
    hdrs = ["catalog.h"],
    deps = [
        ":sql_type",
    ],
)

cc_library(
    name = "catalog_id",
    srcs = ["catalog_id.cc"],
    hdrs = ["catalog_id.h"],
    deps = [
        "@type_safe",
    ],
)

cc_library(
    name = "sql_type",
    hdrs = ["sql_type.h"],
    deps = [],
)
