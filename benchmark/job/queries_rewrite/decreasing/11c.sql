SELECT MIN(cn.name) AS from_company,

       MIN(mc.note) AS production_note,

       MIN(t.title) AS movie_based_on_book

FROM company_name AS cn,

     company_type AS ct,

     keyword AS k,

     link_type AS lt,

     movie_companies AS mc,

     movie_keyword AS mk,

     movie_link AS ml,

     title AS t WHERE mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND t.production_year > 1950 AND t.production_year > 1950 AND t.production_year > 1950 AND t.production_year > 1950 AND t.production_year > 1950 AND t.production_year > 1950 AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND (cn.name LIKE '20th Century Fox%'

       OR cn.name LIKE 'Twentieth Century Fox%') AND cn.country_code <> '[pl]' AND ct.kind IS NOT NULL AND ct.kind <> 'production companies' AND k.keyword IN ('sequel',

                    'revenge',

                    'based-on-novel') AND t.id = mc.movie_id AND t.id = mk.movie_id AND mk.keyword_id = k.id AND ml.movie_id = t.id AND lt.id = ml.link_type_id AND ml.movie_id = mc.movie_id AND mk.movie_id = mc.movie_id AND mc.company_id = cn.id AND mc.company_type_id = ct.id AND ml.movie_id = mk.movie_id;