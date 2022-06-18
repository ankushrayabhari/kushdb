SELECT MIN(t.title) AS american_vhs_movie

FROM company_type AS ct,

     info_type AS it,

     movie_companies AS mc,

     movie_info AS mi,

     title AS t WHERE mi.info IN ('USA',

                  'America') AND mi.info IN ('USA',

                  'America') AND mi.info IN ('USA',

                  'America') AND mi.info IN ('USA',

                  'America') AND mi.info IN ('USA',

                  'America') AND mi.info IN ('USA',

                  'America') AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND mc.note LIKE '%(1994)%' AND mc.note LIKE '%(VHS)%' AND mc.note LIKE '%(USA)%' AND mc.note LIKE '%(1994)%' AND mc.note LIKE '%(1994)%' AND mc.note LIKE '%(1994)%' AND mc.note LIKE '%(1994)%' AND mc.note LIKE '%(1994)%' AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND ct.kind = 'production companies' AND t.id = mi.movie_id AND ct.id = mc.company_type_id AND t.id = mc.movie_id AND mc.movie_id = mi.movie_id AND it.id = mi.info_type_id;