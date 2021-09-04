#!/bin/bash
QUERY=$1
shift
bazel run -c opt --config=profile --run_under="sudo perf record -F 5000 -k 1 -g --call-graph fp -o /tmp/perf.data" $QUERY -- --backend=asm $@
sudo cp /tmp/perf.data perf-asm.data
sudo perf script -i perf-asm.data | ./profile//stackcollapse-perf.pl > /tmp/out.perf-folded
cat /tmp/out.perf-folded | ./profile/flamegraph.pl > perf-asm.svg