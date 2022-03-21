#!/bin/bash
# Example: ./profile/profile_asm.sh bazel-bin/benchmark/tpch/q02_skinner --skinner_join=permute
QUERY=$1
shift
bazel build -c opt --config=profile //...
sudo perf record -F 5000 -k 1 -g --call-graph fp -o /tmp/perf.data $QUERY --backend=asm $@
sudo cp /tmp/perf.data perf-asm.data
sudo perf script -i perf-asm.data | ./profile//stackcollapse-perf.pl > /tmp/out.perf-folded
cat /tmp/out.perf-folded | ./profile/flamegraph.pl > perf-asm.svg