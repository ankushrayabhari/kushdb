import os
import sys
import glob


def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)


def bench(database, flags):
    binary = 'bazel run -c opt --config=comptime --ui_event_filters=-info,-stdout,-stderr --noshow_progress //benchmark/job:query_runner'
    args = ' -- --join_seed=1337'
    for flag in flags:
        args += ' ' + flag
    redirect = ' > /dev/null 2> /tmp/bench_time.txt'

    queries = glob.glob('benchmark/job/queries/*.sql')
    for query in queries:
        execute(binary + args + ' --query=' + query + ' ' + redirect)

        parts = query.split('/')
        query_num = parts[3][:-4]
        with open('/tmp/bench_time.txt', 'r') as file:
            data = file.read().replace('\n', ' ')
            times = []
            for t in data.split():
                try:
                    times.append(float(t))
                except ValueError as e:
                    continue
            it = iter(times)
            times = [t for t in zip(it, it, it)]
            for (t1, t2, t3) in times:
                print(database + ' - ' + query_num, t1, t2, t3, sep=',', flush=True)


if __name__ == "__main__":
    bench('ASM Permute',    ['--use_dictionary=true',  '--skinner_join=permute',   '--backend=asm'])
    bench('ASM Recompile',  ['--use_dictionary=true',  '--skinner_join=recompile', '--backend=asm'])
    bench('Hybrid',         ['--use_dictionary=true',  '--skinner_join=hybrid'])
    bench('LLVM Permute',   ['--use_dictionary=true',  '--skinner_join=permute',   '--backend=llvm'])
    bench('LLVM Recompile', ['--use_dictionary=true',  '--skinner_join=recompile', '--backend=llvm'])