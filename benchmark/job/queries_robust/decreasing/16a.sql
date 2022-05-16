SELECT MIN(an.name) AS cool_actor_pseudonym,

       MIN(t.title) AS series_named_after_char

FROM aka_name AS an,

     cast_info AS ci,

     company_name AS cn,

     keyword AS k,

     movie_companies AS mc,

     movie_keyword AS mk,

     name AS n,

     title AS t WHERE ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND n.id >= 0 AND n.id >= 0 AND n.id >= 0 AND n.id >= 0 AND n.id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND cn.country_code ='[us]' AND t.episode_nr < 100 AND t.episode_nr >= 50 AND k.keyword ='character-name-in-title' AND t.id = mc.movie_id AND ci.movie_id = mk.movie_id AND mc.movie_id = mk.movie_id AND n.id = ci.person_id AND t.id = mk.movie_id AND ci.movie_id = t.id AND an.person_id = ci.person_id AND ci.movie_id = mc.movie_id AND mc.company_id = cn.id AND an.person_id = n.id AND mk.keyword_id = k.id;