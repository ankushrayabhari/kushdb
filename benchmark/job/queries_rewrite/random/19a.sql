SELECT MIN(n.name) AS voicing_actress,

       MIN(t.title) AS voiced_movie

FROM aka_name AS an,

     char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     info_type AS it,

     movie_companies AS mc,

     movie_info AS mi,

     name AS n,

     role_type AS rt,

     title AS t WHERE mc.note IS NOT NULL AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND mc.note IS NOT NULL AND it.info = 'release dates' AND it.info = 'release dates' AND it.info = 'release dates' AND it.info = 'release dates' AND it.info = 'release dates' AND it.info = 'release dates' AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND n.name LIKE '%Ang%' AND n.gender ='f' AND mi.info IS NOT NULL AND (mi.info LIKE 'Japan:%200%'

       OR mi.info LIKE 'USA:%200%') AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND cn.country_code ='[us]' AND t.production_year between 2005 and 2009 AND rt.role ='actress' AND t.id = mi.movie_id AND n.id = an.person_id AND t.id = mc.movie_id AND mi.movie_id = ci.movie_id AND rt.id = ci.role_id AND mc.movie_id = mi.movie_id AND ci.person_id = an.person_id AND it.id = mi.info_type_id AND cn.id = mc.company_id AND n.id = ci.person_id AND t.id = ci.movie_id AND chn.id = ci.person_role_id AND mc.movie_id = ci.movie_id;