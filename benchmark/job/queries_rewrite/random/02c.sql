SELECT MIN(t.title) AS movie_title

FROM company_name AS cn,

     keyword AS k,

     movie_companies AS mc,

     movie_keyword AS mk,

     title AS t WHERE k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND cn.country_code ='[sm]' AND mc.movie_id = mk.movie_id AND t.id = mk.movie_id AND mc.movie_id = t.id AND cn.id = mc.company_id AND mk.keyword_id = k.id;