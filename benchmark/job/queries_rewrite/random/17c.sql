SELECT MIN(n.name) AS member_in_charnamed_movie,

       MIN(n.name) AS a1

FROM cast_info AS ci,

     company_name AS cn,

     keyword AS k,

     movie_companies AS mc,

     movie_keyword AS mk,

     name AS n,

     title AS t WHERE mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND n.name LIKE 'X%' AND n.name LIKE 'X%' AND n.name LIKE 'X%' AND n.name LIKE 'X%' AND n.name LIKE 'X%' AND n.name LIKE 'X%' AND t.id = mc.movie_id AND ci.movie_id = mk.movie_id AND mc.movie_id = mk.movie_id AND n.id = ci.person_id AND t.id = mk.movie_id AND ci.movie_id = t.id AND ci.movie_id = mc.movie_id AND mc.company_id = cn.id AND mk.keyword_id = k.id;