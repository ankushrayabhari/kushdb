Contains the configs necessary for RBE (remote build execution).

Prerequisites:
1. Install docker.
2. Follow the instructions here to get the rbe_configs_gen binary.
   https://github.com/bazelbuild/bazel-toolchains

To update the toolchain:

1. Update Dockerfile. Build it with `docker build -t ankushrayabhari/kushdb_platform .` and upload with `docker push ankushrayabhari/kushdb_platform:latest`.

2. Run `./rbe_configs_gen --toolchain_container=ankushrayabhari/kushdb_platform:latest --output_tarball=rbe_default.tar --exec_os=linux --target_os=linux --bazel_version=4.1.0 -generate_java_configs=false`.

3. Extract the contents of `rbe_default.tar` to the config folder here.

4. Update any reference to `//cc` to `//toolchain/config/cc`.