load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "tpch_schema",
    srcs = ["tpch_schema.cc"],
    deps = [
        "//data:column_data",
        "@absl//absl/time",
    ],
)
