SELECT MIN(an.name) AS acress_pseudonym,

       MIN(t.title) AS japanese_anime_movie

FROM aka_name AS an,

     cast_info AS ci,

     company_name AS cn,

     movie_companies AS mc,

     name AS n,

     role_type AS rt,

     title AS t WHERE rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND t.production_year between 2006 and 2007 AND (t.title LIKE 'One Piece%'

       OR t.title LIKE 'Dragon Ball Z%') AND t.production_year between 2006 and 2007 AND t.production_year between 2006 and 2007 AND t.production_year between 2006 and 2007 AND t.production_year between 2006 and 2007 AND t.production_year between 2006 and 2007 AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND cn.country_code ='[jp]' AND mc.note NOT LIKE '%(USA)%' AND (mc.note LIKE '%(2006)%'

       OR mc.note LIKE '%(2007)%') AND mc.note LIKE '%(Japan)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND mc.note NOT LIKE '%(USA)%' AND n.name NOT LIKE '%Yu%' AND n.name LIKE '%Yo%' AND ci.note ='(voice: English version)' AND ci.role_id = rt.id AND t.id = mc.movie_id AND ci.movie_id = t.id AND an.person_id = ci.person_id AND mc.company_id = cn.id AND ci.movie_id = mc.movie_id AND n.id = ci.person_id AND an.person_id = n.id;