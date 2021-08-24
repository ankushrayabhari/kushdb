Contains the configs necessary for RBE (remote build execution).

To generate the RBE configs:

1. Follow the instructions here to get the rbe_configs_gen binary.
   https://github.com/bazelbuild/bazel-toolchains

2. Run `./rbe_configs_gen --toolchain_container=ankushrayabhari/kushdb_platform:latest --output_tarball=rbe_default.tar --exec_os=linux --target_os=linux --bazel_version=4.1.0 -generate_java_configs=false`.

3. Extract the contents of `rbe_default.tar` to the config folder here.