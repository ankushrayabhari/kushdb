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
    return 'COPY INTO ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".csv") + "\' USING DELIMITERS ',', E'\\n', '\"' NULL AS '';"

if __name__ == "__main__":
    execute('monetdbd create benchmark/job/monetdb')
    execute('monetdbd start benchmark/job/monetdb')

    execute('monetdb create job')
    execute('monetdb set nthreads=1 job')
    execute('monetdb release job')
    execute("mclient -d job benchmark/job/schema.sql")
    text_file = open("/tmp/load.sql", "w")
    for table in tables:
        text_file.write(copy('benchmark/job/raw/', table))
    text_file.close()
    execute("mclient -d job /tmp/load.sql")
    execute('monetdb stop job')

    execute('monetdbd stop benchmark/job/monetdb')