#!/bin/bash
# Example: ./profile/profile_asm.sh bazel-bin/benchmark/tpch/q02_skinner --skinner_join=permute
QUERY=$1
shift
bazel build -c opt --config=profile //...
FILENAME=`date '+%Y-%m-%d-%H-%M'`-`uuidgen -t`
sudo $QUERY --perfpath="/tmp/${FILENAME}.data" --backend=asm $@ >/dev/null
sudo cp "/tmp/${FILENAME}.data" perf-asm.data
sudo perf script -i perf-asm.data | ./profile//stackcollapse-perf.pl > /tmp/out.perf-folded
cat /tmp/out.perf-folded | ./profile/flamegraph.pl > perf-asm.svg