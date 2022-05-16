SELECT MIN(an1.name) AS actress_pseudonym,

       MIN(t.title) AS japanese_movie_dubbed

FROM aka_name AS an1,

     cast_info AS ci,

     company_name AS cn,

     movie_companies AS mc,

     name AS n1,

     role_type AS rt,

     title AS t WHERE rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND n1.name LIKE '%Yo%' AND n1.name NOT LIKE '%Yu%' AND n1.name LIKE '%Yo%' AND n1.name LIKE '%Yo%' AND n1.name LIKE '%Yo%' AND n1.name LIKE '%Yo%' AND n1.name LIKE '%Yo%' AND mc.note NOT LIKE '%(USA)%' AND mc.note LIKE '%(Japan)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND ci.note ='(voice: English version)' AND ci.role_id = rt.id AND t.id = mc.movie_id AND ci.movie_id = t.id AND an1.person_id = ci.person_id AND n1.id = ci.person_id AND mc.company_id = cn.id AND ci.movie_id = mc.movie_id AND an1.person_id = n1.id;