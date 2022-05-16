SELECT MIN(an.name) AS alternative_name,

       MIN(chn.name) AS voiced_char_name,

       MIN(n.name) AS voicing_actress,

       MIN(t.title) AS american_movie

FROM aka_name AS an,

     char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     movie_companies AS mc,

     name AS n,

     role_type AS rt,

     title AS t WHERE t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND rt.role ='actress' AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND mc.company_id >= 0 AND n.gender ='f' AND n.gender ='f' AND n.gender ='f' AND n.gender ='f' AND n.gender ='f' AND n.gender ='f' AND cn.country_code ='[us]' AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND ci.role_id = rt.id AND t.id = mc.movie_id AND ci.movie_id = t.id AND an.person_id = ci.person_id AND mc.company_id = cn.id AND ci.movie_id = mc.movie_id AND n.id = ci.person_id AND an.person_id = n.id AND chn.id = ci.person_role_id;