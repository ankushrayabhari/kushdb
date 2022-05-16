SELECT MIN(cn.name) AS producing_company,

       MIN(lt.link) AS link_type,

       MIN(t.title) AS complete_western_sequel

FROM complete_cast AS cc,

     comp_cast_type AS cct1,

     comp_cast_type AS cct2,

     company_name AS cn,

     company_type AS ct,

     keyword AS k,

     link_type AS lt,

     movie_companies AS mc,

     movie_info AS mi,

     movie_keyword AS mk,

     movie_link AS ml,

     title AS t WHERE mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mc.note IS NULL AND mc.note IS NULL AND mc.note IS NULL AND mc.note IS NULL AND mc.note IS NULL AND mc.note IS NULL AND t.production_year between 1950 and 2000 AND t.production_year between 1950 and 2000 AND t.production_year between 1950 and 2000 AND t.production_year between 1950 and 2000 AND t.production_year between 1950 and 2000 AND t.production_year between 1950 and 2000 AND cc.status_id >= 0 AND cc.status_id >= 0 AND cc.status_id >= 0 AND cc.status_id >= 0 AND cc.status_id >= 0 AND mi.info IN ('Sweden',

                  'Germany',

                  'Swedish',

                  'German') AND (cn.name LIKE '%Film%'

       OR cn.name LIKE '%Warner%') AND cn.country_code <> '[pl]' AND cct1.kind IN ('cast',

                    'crew') AND lt.link LIKE '%follow%' AND cct2.kind = 'complete' AND ct.kind ='production companies' AND k.keyword ='sequel' AND t.id = cc.movie_id AND mk.movie_id = cc.movie_id AND mk.movie_id = mi.movie_id AND cct1.id = cc.subject_id AND mi.movie_id = cc.movie_id AND ml.movie_id = cc.movie_id AND ml.movie_id = mk.movie_id AND mc.movie_id = mi.movie_id AND mi.movie_id = t.id AND t.id = mc.movie_id AND ml.movie_id = t.id AND lt.id = ml.link_type_id AND cct2.id = cc.status_id AND mk.movie_id = mc.movie_id AND mc.company_type_id = ct.id AND mc.movie_id = cc.movie_id AND t.id = mk.movie_id AND ml.movie_id = mc.movie_id AND ml.movie_id = mi.movie_id AND mc.company_id = cn.id AND mk.keyword_id = k.id;