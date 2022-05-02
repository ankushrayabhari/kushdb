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
    bench('kushdb ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=false'])
    bench('kushdb ASM (Join Recompile)', ['--backend=asm', '--skinner_join=recompile', '--use_dictionary=false'])
    bench('kushdb LLVM (Join Permute)', ['--backend=llvm', '--skinner_join=permute', '--use_dictionary=false'])
    bench('kushdb LLVM (Join Recompile)', ['--backend=llvm', '--skinner_join=recompile', '--use_dictionary=false'])
    bench('kushdb DC ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=true'])
    bench('kushdb DC ASM (Join Recompile)', ['--backend=asm', '--skinner_join=recompile', '--use_dictionary=true'])
    bench('kushdb DC LLVM (Join Permute)', ['--backend=llvm', '--skinner_join=permute', '--use_dictionary=true'])
    bench('kushdb DC LLVM (Join Recompile)', ['--backend=llvm', '--skinner_join=recompile', '--use_dictionary=true'])
