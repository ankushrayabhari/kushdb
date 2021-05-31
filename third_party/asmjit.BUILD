load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "asmjit",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["src/**/*.h"]),
    includes = ["src"],
    linkopts = ["-lrt"],
    deps = [],
)
