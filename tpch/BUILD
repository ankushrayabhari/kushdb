load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "load",
    srcs = ["load.cc"],
    deps = [
        "//data:column_data",
        "@absl//absl/time",
    ],
)

cc_binary(
    name = "schema",
    hdrs = ["schema.h"],
    deps = [
        "//catalog",
        "//catalog:sql_type",
    ],
)
