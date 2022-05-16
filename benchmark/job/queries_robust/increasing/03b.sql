SELECT MIN(t.title) AS movie_title

FROM keyword AS k,

     movie_info AS mi,

     movie_keyword AS mk,

     title AS t WHERE k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND mi.info IN ('Bulgaria') AND mi.info IN ('Bulgaria') AND mi.info IN ('Bulgaria') AND mi.info IN ('Bulgaria') AND mi.info IN ('Bulgaria') AND mi.info IN ('Bulgaria') AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND t.id = mk.movie_id AND t.id = mi.movie_id AND mk.movie_id = mi.movie_id AND k.id = mk.keyword_id;