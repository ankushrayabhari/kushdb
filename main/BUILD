load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "rundylib",
    srcs = ["rundylib.cc"],
    linkopts = ["-ldl"],
    deps = [],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        "//algebra:operator",
        "//compilation:cpp_translator",
    ],
)

cc_binary(
    name = "generate_vector",
    srcs = ["generate_vector.cc"],
    deps = [
        "//data:column_data",
    ],
)

cc_binary(
    name = "read_vector",
    srcs = ["read_vector.cc"],
    deps = [
        "//data:column_data",
    ],
)
