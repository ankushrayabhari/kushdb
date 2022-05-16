import math
import random
import re
import os
import sys

def read_queries(query_path):
    with open(query_path) as f:
        lines = f.readlines()
        return "\n".join(lines)

def load_queries(path):
    queries = {}
    for file in os.listdir(path):
        if file.endswith(".sql"):
            query_path = os.path.join(path, file)
            query = read_queries(query_path)
            queries[file] = query
    return queries

def query_graph(query):
    tables = [list(map(lambda x: x.strip(), exp.split("AS"))) for exp in re.findall(r"\w+ AS \w+", query)]
    alias_set = [table[1] for table in tables]
    # Find BETWEEN left AND right
    betweens = set(re.findall(r"BETWEEN \d+ AND \d+", query) + re.findall(r"BETWEEN '\w+' AND '\w+'", query))
    for between in betweens:
        replacement = between.replace("BETWEEN", "between").replace("AND", "and")
        query = query.replace(between, replacement)
    predicates = list(map(lambda x: x.strip(), query.split("WHERE")[1].replace(";", "").split(" AND ")))
    combine = False
    nr_left = 0
    nr_right = 0
    combine_list = []
    added_predicates = []
    removed_predicates = []
    for predicate in predicates:
        nr_left += predicate.count('(')
        nr_right += predicate.count(')')
        if nr_left != nr_right:
            combine = True
            combine_list.append(predicate)
            removed_predicates.append(predicate)
        if combine and nr_left == nr_right:
            combine = False
            combine_list.append(predicate)
            removed_predicates.append(predicate)
            combine_predicates = " AND ".join(combine_list)
            added_predicates.append(combine_predicates)
            combine_list = []
            nr_left = 0
            nr_right = 0

    for predicate in removed_predicates:
        predicates.remove(predicate)

    for predicate in added_predicates:
        predicates.append(predicate)

    predicates = set(predicates)
    selects = query.split("WHERE")[0].strip()
    unary_predicates_alias = {alias: [] for alias in alias_set}
    join_predicates_alias = {alias: [] for alias in alias_set}
    join_columns = {alias: set() for alias in alias_set}
    join_predicates = set(filter(lambda exp: re.match(r"\w+\.\w+\s*=\s*\w+\.\w+", exp) is not None, predicates))
    unary_predicates = predicates.difference(join_predicates)

    # Add unary predicates to a map
    for unary_predicate in unary_predicates:
        unary_table = re.findall(r"\w+\.\w+", unary_predicate)[0]
        unary_alias = unary_table.split(".")[0].strip()
        unary_predicates_alias[unary_alias].append(unary_predicate)

    # Join graph
    for join_predicate in join_predicates:
        tables = join_predicate.split("=")
        left = tables[0].strip().split(".")[0]
        right = tables[1].strip().split(".")[0]
        left_col = tables[0].strip().split(".")[1]
        right_col = tables[1].strip().split(".")[1]
        join_predicates_alias[left].append(right)
        join_predicates_alias[right].append(left)
        join_columns[left].add(left_col)
        join_columns[right].add(right_col)
    return unary_predicates_alias, join_predicates_alias, join_columns, selects, list(join_predicates)


def retrieve_cards(path):
    cards_map = {}
    current_query = False
    with open(path) as f:
        for line in f.readlines():
            line = line.strip()
            if line.endswith(".sql"):
                query_name = line
                current_query = True
            elif line.startswith("Alias:") and current_query:
                alias = line.split(": ")[1][1:-1]
                alias_list = alias.split(",")
                alias = [x.strip() for x in alias_list]
            elif line.startswith("Cards:") and current_query:
                current_query = False
                cards = line.split(": ")[1][1:-1]
                card_list = cards.split(",")
                cards = [int(x.strip()) for x in card_list]
                zip_iterator = zip(alias, cards)
                card_dict = dict(zip_iterator)
                cards_map[query_name] = card_dict
    return cards_map

def generate_duplicate_queries(query, cards):
    alias_list = list(query_graph(query)[0].keys())
    if cards is None:
        print(query)
        return []

    increasing_order = sorted(alias_list.copy(), key=lambda x: cards[x])
    decreasing_order = sorted(alias_list.copy(), key=lambda x: cards[x], reverse=True)
    random_order = alias_list.copy()
    random.shuffle(random_order)

    duplicates = [query]
    for query_order in [increasing_order, decreasing_order, random_order]:
        nr_predicates = 0
        nr_duplicates = 5
        total_predicates = 20

        unary_predicates, _, join_columns, selects, joins = query_graph(query)
        unarys = []
        for table_alias in query_order:
            table_unary_predicates = unary_predicates[table_alias]
            if nr_predicates < total_predicates:
                for d in range(nr_duplicates):
                    if len(table_unary_predicates) == 0:
                        join_column = next(iter(join_columns[table_alias]))
                        table_unary_predicates.append(f"{table_alias}.{join_column} >= 0")
                    else:
                        table_unary_predicates.append(table_unary_predicates[0])
                nr_predicates += nr_duplicates
            unarys += table_unary_predicates

        du_unarys = " AND ".join(unarys)
        du_joins = " AND ".join(joins)
        duplicate_query = f"{selects} WHERE {du_unarys} AND {du_joins};"
        duplicates.append(duplicate_query)
    return duplicates


def shuffle_duplicates():
    output_dirs = [
        f"benchmark/job/queries_robust/original/",
        f"benchmark/job/queries_robust/increasing/",
        f"benchmark/job/queries_robust/decreasing/",
        f"benchmark/job/queries_robust/random/"
    ]
    for output_prefix in output_dirs:
        os.makedirs(output_prefix, exist_ok=True)

    card_path = "benchmark/job/cards.log"
    cards_queries = retrieve_cards(card_path)

    query_path = f"benchmark/job/queries/"
    queries = load_queries(query_path)
    query_keys = sorted(queries.keys())
    for query_key in query_keys:
        print(f"Constructing query {query_key}...")
        cards = cards_queries[query_key] if query_key in cards_queries else None
        duplicates = generate_duplicate_queries(queries[query_key], cards)
        for idx, duplicate in enumerate(duplicates):
            with open(f"{output_dirs[idx]}{query_key}", 'w') as f:
                f.write(duplicate)

shuffle_duplicates()