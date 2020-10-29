load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "type_safe",
    hdrs = glob(["include/**/*.hpp"]),
    strip_include_prefix = "include",
)
