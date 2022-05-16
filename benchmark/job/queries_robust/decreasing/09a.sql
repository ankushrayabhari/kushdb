SELECT MIN(an.name) AS alternative_name,

       MIN(chn.name) AS character_name,

       MIN(t.title) AS movie

FROM aka_name AS an,

     char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     movie_companies AS mc,

     name AS n,

     role_type AS rt,

     title AS t WHERE chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND t.production_year between 2005 and 2015 AND t.production_year between 2005 and 2015 AND t.production_year between 2005 and 2015 AND t.production_year between 2005 and 2015 AND t.production_year between 2005 and 2015 AND t.production_year between 2005 and 2015 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND mc.note IS NOT NULL AND cn.country_code ='[us]' AND n.name LIKE '%Ang%' AND n.gender ='f' AND rt.role ='actress' AND ci.role_id = rt.id AND t.id = mc.movie_id AND ci.movie_id = t.id AND an.person_id = ci.person_id AND mc.company_id = cn.id AND ci.movie_id = mc.movie_id AND n.id = ci.person_id AND an.person_id = n.id AND chn.id = ci.person_role_id;