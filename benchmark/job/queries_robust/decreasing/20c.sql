SELECT MIN(n.name) AS cast_member,

       MIN(t.title) AS complete_dynamic_hero_movie

FROM complete_cast AS cc,

     comp_cast_type AS cct1,

     comp_cast_type AS cct2,

     char_name AS chn,

     cast_info AS ci,

     keyword AS k,

     kind_type AS kt,

     movie_keyword AS mk,

     name AS n,

     title AS t WHERE ci.person_role_id >= 0 AND ci.person_role_id >= 0 AND ci.person_role_id >= 0 AND ci.person_role_id >= 0 AND ci.person_role_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND n.id >= 0 AND n.id >= 0 AND n.id >= 0 AND n.id >= 0 AND n.id >= 0 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND (chn.name LIKE '%man%'

       OR chn.name LIKE '%Man%') AND chn.name IS NOT NULL AND k.keyword IN ('superhero',

                    'marvel-comics',

                    'based-on-comic',

                    'tv-special',

                    'fight',

                    'violence',

                    'magnet',

                    'web',

                    'claw',

                    'laser') AND cct2.kind LIKE '%complete%' AND cct1.kind = 'cast' AND kt.kind = 'movie' AND t.id = cc.movie_id AND kt.id = t.kind_id AND mk.movie_id = cc.movie_id AND ci.movie_id = cc.movie_id AND t.id = mk.movie_id AND mk.movie_id = ci.movie_id AND cct2.id = cc.status_id AND n.id = ci.person_id AND cct1.id = cc.subject_id AND t.id = ci.movie_id AND chn.id = ci.person_role_id AND k.id = mk.keyword_id;