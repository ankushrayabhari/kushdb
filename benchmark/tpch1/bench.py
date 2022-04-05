import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def bench(database, flags):
    queries = ['q01', 'q02', 'q03', 'q05', 'q06', 'q07', 'q08', 'q09', 'q10',
               'q11', 'q12', 'q14', 'q18', 'q19']
    for query in queries:
        binary = 'bazel run -c opt --ui_event_filters=-info,-stdout,-stderr --noshow_progress //benchmark/tpch1/queries:' + query

        binary += ' -- '
        for f in flags:
            binary += ' ' + f
        redirect = ' > /dev/null 2> /tmp/bench_time.txt'
        executable = binary + redirect

        execute(executable)

        query_num = int(query[1:])

        with open('/tmp/bench_time.txt', 'r') as file:
            data = file.read().replace('\n', ' ')
            times = []
            for t in data.split():
                try:
                    times.append(float(t))
                except ValueError as e:
                    continue
            for t in times:
                print(database, 'TPC-H SF1', query_num, t, sep=',')

if __name__ == "__main__":
    bench('kushdb ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--skinner_scan_select=none'])
    bench('kushdb ASM (Join Recompile)', ['--backend=asm', '--skinner_join=recompile', '--skinner_scan_select=none'])
    bench('kushdb LLVM (Join Permute)', ['--backend=llvm', '--skinner_join=permute', '--skinner_scan_select=none'])
    bench('kushdb LLVM (Join Recompile)', ['--backend=llvm', '--skinner_join=recompile', '--skinner_scan_select=none'])
    #bench('kushdb ASM (Join Permute|Scan/Select Permute)', ['--backend=asm', '--skinner_join=permute', '--skinner_scan_select=permute'])
    #bench('kushdb ASM (Join Recompile|Scan/Select Permute)', ['--backend=asm', '--skinner_join=recompile', '--skinner_scan_select=permute'])
    #bench('kushdb LLVM (Join Permute|Scan/Select Permute)', ['--backend=llvm', '--skinner_join=permute', '--skinner_scan_select=permute'])
    #bench('kushdb LLVM (Join Recompile|Scan/Select Permute)', ['--backend=llvm', '--skinner_join=recompile', '--skinner_scan_select=permute'])