import os
import sys
import glob


def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)


def bench(database, flags):
    binary = 'bazel run -c opt --ui_event_filters=-info,-stdout,-stderr --noshow_progress //benchmark/job:query_runner'
    args = ' -- '
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
            for t in times:
                print(database, 'JOB', query_num, t, sep=',')


if __name__ == "__main__":
    bench('kushdb (ASM Permute)',       ['--use_dictionary=false', '--skinner_join=permute',   '--backend=asm'])
    bench('kushdb (ASM Recompile)',     ['--use_dictionary=false', '--skinner_join=recompile', '--backend=asm'])
    bench('kushdb (LLVM Permute)',      ['--use_dictionary=false', '--skinner_join=permute',   '--backend=llvm'])
    bench('kushdb (LLVM Recompile)',    ['--use_dictionary=false', '--skinner_join=recompile', '--backend=llvm'])
    bench('kushdb (Hybrid)',            ['--use_dictionary=false', '--skinner_join=hybrid'])
    bench('kushdb DC (ASM Permute)',    ['--use_dictionary=true',  '--skinner_join=permute',   '--backend=asm'])
    bench('kushdb DC (ASM Recompile)',  ['--use_dictionary=true',  '--skinner_join=recompile', '--backend=asm'])
    bench('kushdb DC (LLVM Permute)',   ['--use_dictionary=true',  '--skinner_join=permute',   '--backend=llvm'])
    bench('kushdb DC (LLVM Recompile)', ['--use_dictionary=true',  '--skinner_join=recompile', '--backend=llvm'])
    bench('kushdb DC (Hybrid)',         ['--use_dictionary=true',  '--skinner_join=hybrid'])