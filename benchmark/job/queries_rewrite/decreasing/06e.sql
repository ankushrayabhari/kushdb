SELECT MIN(k.keyword) AS movie_keyword,

       MIN(n.name) AS actor_name,

       MIN(t.title) AS marvel_movie

FROM cast_info AS ci,

     keyword AS k,

     movie_keyword AS mk,

     name AS n,

     title AS t WHERE ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND n.name LIKE '%Downey%Robert%' AND n.name LIKE '%Downey%Robert%' AND n.name LIKE '%Downey%Robert%' AND n.name LIKE '%Downey%Robert%' AND n.name LIKE '%Downey%Robert%' AND n.name LIKE '%Downey%Robert%' AND k.keyword = 'marvel-cinematic-universe' AND ci.movie_id = mk.movie_id AND t.id = mk.movie_id AND n.id = ci.person_id AND k.id = mk.keyword_id AND t.id = ci.movie_id;