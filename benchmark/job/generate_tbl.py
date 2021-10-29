import os
import sys

tables = ['aka_name', 'aka_title', 'cast_info', 'char_name', 'comp_cast_type',
          'company_name', 'company_type', 'complete_cast', 'info_type',
          'keyword', 'kind_type', 'link_type', 'movie_companies', 'movie_info',
          'movie_info_idx', 'movie_keyword', 'movie_link', 'name',
          'person_info', 'role_type', 'title']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def generate(tbl):
    return [
        '.mode csv', ".separator '|'", '.headers off',
        '.output benchmark/job/raw/' + tbl + '.tbl',
        'SELECT * FROM ' + tbl + ';',
    ]

def execute_duckdb(cmds):
    with open ('/tmp/duckdb.txt', 'w') as f:
        for cmd in cmds:
            print(cmd, file=sys.stderr)
            print(cmd, file=f)
    execute('duckdb benchmark/job/duckdb/job.ddb < /tmp/duckdb.txt')

def clean(tbl):
    execute("sed -i 's/\\r$//' benchmark/job/raw/" + tbl + '.tbl')

if __name__ == "__main__":
    for tbl in tables:
        execute_duckdb(generate(tbl))
    for tbl in tables:
        clean(tbl)
