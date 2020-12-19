select
	ps_partkey,
	sum(ps_supplycost * ps_availqty) as value
from
	partsupp_11,
	supplier_11,
	nation_11
where
	ps_suppkey = s_suppkey
	and s_nationkey = n_nationkey
	and n_name = 'ARGENTINA'
group by
	ps_partkey;