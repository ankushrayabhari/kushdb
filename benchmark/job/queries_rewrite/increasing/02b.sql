SELECT MIN(t.title) AS movie_title

FROM company_name AS cn,

     keyword AS k,

     movie_companies AS mc,

     movie_keyword AS mk,

     title AS t WHERE k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND k.keyword ='character-name-in-title' AND cn.country_code ='[nl]' AND cn.country_code ='[nl]' AND cn.country_code ='[nl]' AND cn.country_code ='[nl]' AND cn.country_code ='[nl]' AND cn.country_code ='[nl]' AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.movie_id = mk.movie_id AND t.id = mk.movie_id AND mc.movie_id = t.id AND cn.id = mc.company_id AND mk.keyword_id = k.id;