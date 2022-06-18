SELECT MIN(n.name) AS of_person,

       MIN(t.title) AS biography_movie

FROM aka_name AS an,

     cast_info AS ci,

     info_type AS it,

     link_type AS lt,

     movie_link AS ml,

     name AS n,

     person_info AS pi,

     title AS t WHERE n.name_pcode_cf between 'A' and 'F' AND (n.gender='m'

       OR (n.gender = 'f' AND n.name LIKE 'B%')) AND n.name_pcode_cf between 'A' and 'F' AND n.name_pcode_cf between 'A' and 'F' AND n.name_pcode_cf between 'A' and 'F' AND n.name_pcode_cf between 'A' and 'F' AND n.name_pcode_cf between 'A' and 'F' AND t.production_year between 1980 and 1995 AND t.production_year between 1980 and 1995 AND t.production_year between 1980 and 1995 AND t.production_year between 1980 and 1995 AND t.production_year between 1980 and 1995 AND t.production_year between 1980 and 1995 AND lt.link ='features' AND lt.link ='features' AND lt.link ='features' AND lt.link ='features' AND lt.link ='features' AND lt.link ='features' AND pi.note ='Volker Boehm' AND pi.note ='Volker Boehm' AND pi.note ='Volker Boehm' AND pi.note ='Volker Boehm' AND pi.note ='Volker Boehm' AND pi.note ='Volker Boehm' AND an.name LIKE '%a%' AND it.info ='mini biography' AND n.id = pi.person_id AND n.id = an.person_id AND ci.person_id = n.id AND it.id = pi.info_type_id AND pi.person_id = ci.person_id AND an.person_id = ci.person_id AND pi.person_id = an.person_id AND lt.id = ml.link_type_id AND ci.movie_id = ml.linked_movie_id AND t.id = ci.movie_id AND ml.linked_movie_id = t.id;