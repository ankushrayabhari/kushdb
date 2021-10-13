FROM ubuntu:20.04
RUN apt-get update
RUN apt install -y apt-transport-https curl gnupg
RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
RUN mv bazel.gpg /etc/apt/trusted.gpg.d/
RUN echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
RUN apt-get install -y clang-10 llvm-10-dev
RUN ln -s /usr/bin/clang-10 /usr/bin/clang
RUN ln -s /usr/bin/clang++-10 /usr/bin/clang++
RUN ln -s /usr/bin/clang-10 /usr/local/bin/clang
RUN ln -s /usr/bin/clang++-10 /usr/local/bin/clang++
RUN apt-get install -y libtbb-dev