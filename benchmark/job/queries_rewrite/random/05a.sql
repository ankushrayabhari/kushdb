SELECT MIN(t.title) AS typical_european_movie

FROM company_type AS ct,

     info_type AS it,

     movie_companies AS mc,

     movie_info AS mi,

     title AS t WHERE t.production_year > 2005 AND t.production_year > 2005 AND t.production_year > 2005 AND t.production_year > 2005 AND t.production_year > 2005 AND t.production_year > 2005 AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND mi.info IN ('Sweden',

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

                  'German') AND mc.note LIKE '%(France)%' AND mc.note LIKE '%(theatrical)%' AND t.id = mi.movie_id AND ct.id = mc.company_type_id AND t.id = mc.movie_id AND mc.movie_id = mi.movie_id AND it.id = mi.info_type_id;