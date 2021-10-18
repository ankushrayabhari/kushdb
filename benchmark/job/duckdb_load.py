import os
import sys


tables = [
    'aka_name', 'aka_title', 'cast_info', 'char_name', 'comp_cast_type',
    'company_name', 'company_type', 'complete_cast', 'info_type',
    'keyword', 'kind_type', 'link_type', 'movie_companies', 'movie_info',
    'movie_info_idx', 'movie_keyword', 'movie_link', 'name',
    'person_info', 'role_type', 'title'
]

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def copy(raw, table):
    return 'COPY ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".csv") + "\' WITH DELIMITER ',' ESCAPE '\\';"

def execute_duckdb(cmds):
    with open ('/tmp/duckdb.txt', 'w') as f:
        for cmd in cmds:
            print(cmd, file=sys.stderr)
            print(cmd, file=f)
    execute('duckdb < /tmp/duckdb.txt')

if __name__ == "__main__":
    cmds = ['.open benchmark/job/duckdb/job.ddb', '.read benchmark/job/schema.sql']
    for table in tables:
        cmds.append(copy('benchmark/job/raw/', table))
    execute_duckdb(cmds)