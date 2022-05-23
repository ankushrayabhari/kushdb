SELECT MIN(mi_idx.info) AS rating,

       MIN(t.title) AS movie_title

FROM info_type AS it,

     keyword AS k,

     movie_info_idx AS mi_idx,

     movie_keyword AS mk,

     title AS t WHERE it.info ='rating' AND it.info ='rating' AND it.info ='rating' AND it.info ='rating' AND it.info ='rating' AND it.info ='rating' AND mi_idx.info > '9.0' AND mi_idx.info > '9.0' AND mi_idx.info > '9.0' AND mi_idx.info > '9.0' AND mi_idx.info > '9.0' AND mi_idx.info > '9.0' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND k.keyword LIKE '%sequel%' AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.id = mk.movie_id AND it.id = mi_idx.info_type_id AND mk.movie_id = mi_idx.movie_id AND t.id = mi_idx.movie_id AND k.id = mk.keyword_id;