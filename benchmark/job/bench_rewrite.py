import os
import sys
import glob


def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)


def bench(benchmark, queries_path, database, flags):
    binary = 'bazel run -c opt --ui_event_filters=-info,-stdout,-stderr --noshow_progress //benchmark/job:query_runner'
    args = ' -- '
    for flag in flags:
        args += ' ' + flag
    redirect = ' > /dev/null 2> /tmp/bench_time.txt'

    queries = glob.glob(queries_path + '*.sql')
    for query in queries:
        execute(binary + args + ' --query=' + query + ' ' + redirect)

        parts = query.split('/')
        query_num = parts[4][:-4]
        with open('/tmp/bench_time.txt', 'r') as file:
            data = file.read().replace('\n', ' ')
            times = []
            for t in data.split():
                try:
                    times.append(float(t))
                except ValueError as e:
                    continue
            for t in times:
                print(database, benchmark, query_num, t, sep=',')


if __name__ == "__main__":
    bench("JOB (decreasing)", "benchmark/job/queries_rewrite/decreasing/", 'kushdb ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=false'])
    bench("JOB (decreasing)", "benchmark/job/queries_rewrite/decreasing/", 'kushdb DC ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=true'])

    bench("JOB (random)", "benchmark/job/queries_rewrite/random/", 'kushdb ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=false'])
    bench("JOB (random)", "benchmark/job/queries_rewrite/random/", 'kushdb DC ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=true'])

    bench("JOB (original)", "benchmark/job/queries_rewrite/original/", 'kushdb ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=false'])
    bench("JOB (original)", "benchmark/job/queries_rewrite/original/", 'kushdb DC ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=true'])

    bench("JOB (increasing)", "benchmark/job/queries_rewrite/increasing/", 'kushdb ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=false'])
    bench("JOB (increasing)", "benchmark/job/queries_rewrite/increasing/", 'kushdb DC ASM (Join Permute)', ['--backend=asm', '--skinner_join=permute', '--use_dictionary=true'])
