#!/bin/bash
python benchmark/tpch1/bench.py > tpch.csv
python benchmark/tpch10/bench.py >> tpch.csv
python benchmark/jcch1/bench.py > jcch.csv
python benchmark/jcch10/bench.py >> jcch.csv