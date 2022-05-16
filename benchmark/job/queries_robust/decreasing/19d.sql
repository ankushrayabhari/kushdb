SELECT MIN(n.name) AS voicing_actress,

       MIN(t.title) AS jap_engl_voiced_movie

FROM aka_name AS an,

     char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     info_type AS it,

     movie_companies AS mc,

     movie_info AS mi,

     name AS n,

     role_type AS rt,

     title AS t WHERE mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND t.production_year > 2000 AND n.gender ='f' AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND cn.country_code ='[us]' AND it.info = 'release dates' AND rt.role ='actress' AND t.id = mi.movie_id AND n.id = an.person_id AND t.id = mc.movie_id AND mi.movie_id = ci.movie_id AND rt.id = ci.role_id AND n.id = ci.person_id AND mc.movie_id = mi.movie_id AND ci.person_id = an.person_id AND it.id = mi.info_type_id AND cn.id = mc.company_id AND t.id = ci.movie_id AND chn.id = ci.person_role_id AND mc.movie_id = ci.movie_id;