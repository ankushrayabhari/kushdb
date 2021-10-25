load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "transform",
    srcs = [
        "transform_column_ref_expression.cc",
        "transform_expression.cc",
        "transform_from.cc",
        "transform_group_by.cc",
        "transform_literal_expression.cc",
        "transform_order_by.cc",
        "transform_select_statement.cc",
        "transform_statement.cc",
    ],
    hdrs = [
        "transform_column_ref_expression.h",
        "transform_expression.h",
        "transform_from.h",
        "transform_group_by.h",
        "transform_literal_expression.h",
        "transform_order_by.h",
        "transform_select_statement.h",
        "transform_statement.h",
    ],
    deps = [
        "//parse/expression",
        "//parse/expression:column_ref_expression",
        "//parse/expression:literal_expression",
        "//parse/statement",
        "//parse/statement:select_statement",
        "//parse/table",
        "//third_party/duckdb_libpgquery",
        "@magic_enum",
    ],
)
