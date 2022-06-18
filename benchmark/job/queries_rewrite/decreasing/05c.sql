SELECT MIN(t.title) AS american_movie

FROM company_type AS ct,

     info_type AS it,

     movie_companies AS mc,

     movie_info AS mi,

     title AS t WHERE t.production_year > 1990 AND t.production_year > 1990 AND t.production_year > 1990 AND t.production_year > 1990 AND t.production_year > 1990 AND t.production_year > 1990 AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German',

                  'USA',

                  'American') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German',

                  'USA',

                  'American') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German',

                  'USA',

                  'American') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German',

                  'USA',

                  'American') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German',

                  'USA',

                  'American') AND mi.info IN ('Sweden',

                  'Norway',

                  'Germany',

                  'Denmark',

                  'Swedish',

                  'Denish',

                  'Norwegian',

                  'German',

                  'USA',

                  'American') AND mc.note LIKE '%(USA)%' AND mc.note NOT LIKE '%(TV)%' AND mc.note LIKE '%(USA)%' AND mc.note LIKE '%(USA)%' AND mc.note LIKE '%(USA)%' AND mc.note LIKE '%(USA)%' AND mc.note LIKE '%(USA)%' AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND it.id >= 0 AND ct.kind = 'production companies' AND t.id = mi.movie_id AND ct.id = mc.company_type_id AND t.id = mc.movie_id AND mc.movie_id = mi.movie_id AND it.id = mi.info_type_id;