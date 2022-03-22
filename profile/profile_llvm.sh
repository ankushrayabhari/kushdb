#!/bin/bash
# Example: ./profile/profile_llvm.sh bazel-bin/benchmark/tpch/q02_skinner --skinner_join=permute
QUERY=$1
shift
bazel build -c opt --config=profile //...
sudo perf record -F 5000 -k 1 -g --call-graph fp -o /tmp/perf.data $QUERY --backend=llvm $@ >/dev/null
sudo perf inject -j -i /tmp/perf.data -o perf-llvm.data
sudo perf script -i perf-llvm.data | ./profile/stackcollapse-perf.pl > /tmp/out.perf-folded
cat /tmp/out.perf-folded | ./profile/flamegraph.pl > perf-llvm.svg