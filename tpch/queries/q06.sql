select
	sum(l_extendedprice * l_discount) as revenue
from
	lineitem
where
	l_shipdate >= date '1994-01-01'
	and l_shipdate < date '1995-01-01'
	and l_discount >= 0.07 
    and l_discount <= 0.09 
	and l_quantity < 24
group by 
	1;