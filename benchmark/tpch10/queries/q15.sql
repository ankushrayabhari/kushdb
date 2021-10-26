


select
	s_suppkey,
	s_name,
	s_address,
	s_phone,
	total_revenue
from
	supplier,
	(select
				l_suppkey,
				sum(l_extendedprice * (1 - l_discount))
			from
				lineitem
			where
				l_shipdate >= date '1997-06-01'
				and l_shipdate < date '1997-06-01' + interval '3' month
			group by
				l_suppkey) revenue0
where
	s_suppkey = supplier_no
	and total_revenue = (
		select
			max(total_revenue)
		from
			(select
				l_suppkey,
				sum(l_extendedprice * (1 - l_discount))
			from
				lineitem
			where
				l_shipdate >= date '1997-06-01'
				and l_shipdate < date '1997-06-01' + interval '3' month
			group by
				l_suppkey) revenue0
	)
order by
	s_suppkey;