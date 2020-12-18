select
	l_shipmode
from
	lineitem
where
	(l_shipmode = 'FOB' or l_shipmode = 'SHIP') 
    and l_commitdate < l_receiptdate 
    and l_shipdate < l_commitdate 
    and l_receiptdate >= date '1994-01-01' 
    and l_receiptdate < '1995-01-01'
group by
	l_shipmode;