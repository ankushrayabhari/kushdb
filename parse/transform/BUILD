load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "transform",
    srcs = [
        "transform_alias.cc",
        "transform_from.cc",
        "transform_order_by.cc",
        "transform_select_statement.cc",
        "transform_statement.cc",
    ],
    hdrs = [
        "transform_alias.h",
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
