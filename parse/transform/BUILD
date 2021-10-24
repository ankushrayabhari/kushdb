load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "transform",
    srcs = ["transform_statement.cc"],
    hdrs = [
        "transform_expression.h",
        "transform_from.h",
        "transform_group_by.h",
        "transform_order_by.h",
        "transform_select_statement.h",
        "transform_statement.h",
    ],
    deps = [
        "//parse/statement",
        "//parse/statement:select_statement",
        "//parse/table",
        "//third_party/duckdb_libpgquery",
        "@magic_enum",
    ],
)
