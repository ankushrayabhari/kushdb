SELECT MIN(cn.name) AS company_name,

       MIN(lt.link) AS link_type,

       MIN(t.title) AS western_follow_up

FROM company_name AS cn,

     company_type AS ct,

     keyword AS k,

     link_type AS lt,

     movie_companies AS mc,

     movie_info AS mi,

     movie_keyword AS mk,

     movie_link AS ml,

     title AS t WHERE mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND lt.link LIKE '%follow%' AND lt.link LIKE '%follow%' AND lt.link LIKE '%follow%' AND lt.link LIKE '%follow%' AND lt.link LIKE '%follow%' AND lt.link LIKE '%follow%' AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German') AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND k.keyword ='sequel' AND (cn.name LIKE '%Film%'

       OR cn.name LIKE '%Warner%') AND cn.country_code <> '[pl]' AND t.production_year between 1950 and 2000 AND mc.note IS NULL AND ct.kind ='production companies' AND t.id = mc.movie_id AND t.id = mk.movie_id AND mk.keyword_id = k.id AND mc.movie_id = mi.movie_id AND mk.movie_id = mi.movie_id AND ml.movie_id = t.id AND mc.company_id = cn.id AND lt.id = ml.link_type_id AND ml.movie_id = mc.movie_id AND ml.movie_id = mi.movie_id AND mk.movie_id = mc.movie_id AND mi.movie_id = t.id AND mc.company_type_id = ct.id AND ml.movie_id = mk.movie_id;