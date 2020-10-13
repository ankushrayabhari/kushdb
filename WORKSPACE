load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# External Dependency Setup
http_archive(
    name = "rules_foreign_cc",
    sha256 = "c823078385e9f6891f7e245be4eb319d6d74f1a3d6d5ba7392f1e382ef190651",
    strip_prefix = "rules_foreign_cc-master",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/master.zip",
)

load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

# External Dependencies

http_archive(
    name = "absl",
    sha256 = "f342aac71a62861ac784cadb8127d5a42c6c61ab1cd07f00aef05f2cc4988c42",
    strip_prefix = "abseil-cpp-20200225.2",
    urls = ["https://github.com/abseil/abseil-cpp/archive/20200225.2.zip"],
)
