SELECT MIN(n.name) AS cast_member_name,

       MIN(pi.info) AS cast_member_info

FROM aka_name AS an,

     cast_info AS ci,

     info_type AS it,

     link_type AS lt,

     movie_link AS ml,

     name AS n,

     person_info AS pi,

     title AS t WHERE ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND ci.person_id >= 0 AND t.production_year between 1980 and 2010 AND t.production_year between 1980 and 2010 AND t.production_year between 1980 and 2010 AND t.production_year between 1980 and 2010 AND t.production_year between 1980 and 2010 AND t.production_year between 1980 and 2010 AND (an.name LIKE '%a%'

       OR an.name LIKE 'A%') AND an.name IS NOT NULL AND (an.name LIKE '%a%'

       OR an.name LIKE 'A%') AND (an.name LIKE '%a%'

       OR an.name LIKE 'A%') AND (an.name LIKE '%a%'

       OR an.name LIKE 'A%') AND (an.name LIKE '%a%'

       OR an.name LIKE 'A%') AND (an.name LIKE '%a%'

       OR an.name LIKE 'A%') AND (n.gender='m'

       OR (n.gender = 'f' AND n.name LIKE 'A%')) AND n.name_pcode_cf between 'A' and 'F' AND (n.gender='m'

       OR (n.gender = 'f' AND n.name LIKE 'A%')) AND (n.gender='m'

       OR (n.gender = 'f' AND n.name LIKE 'A%')) AND (n.gender='m'

       OR (n.gender = 'f' AND n.name LIKE 'A%')) AND (n.gender='m'

       OR (n.gender = 'f' AND n.name LIKE 'A%')) AND (n.gender='m'

       OR (n.gender = 'f' AND n.name LIKE 'A%')) AND pi.note IS NOT NULL AND lt.link IN ('references',

                  'referenced in',

                  'features',

                  'featured in') AND it.info ='mini biography' AND n.id = pi.person_id AND n.id = an.person_id AND ci.person_id = n.id AND it.id = pi.info_type_id AND pi.person_id = ci.person_id AND an.person_id = ci.person_id AND pi.person_id = an.person_id AND lt.id = ml.link_type_id AND ci.movie_id = ml.linked_movie_id AND t.id = ci.movie_id AND ml.linked_movie_id = t.id;