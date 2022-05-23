SELECT MIN(chn.name) AS voiced_char_name,

       MIN(n.name) AS voicing_actress_name,

       MIN(t.title) AS kung_fu_panda

FROM aka_name AS an,

     char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     info_type AS it,

     keyword AS k,

     movie_companies AS mc,

     movie_info AS mi,

     movie_keyword AS mk,

     name AS n,

     role_type AS rt,

     title AS t WHERE mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND (mi.info LIKE 'Japan:%201%'

       OR mi.info LIKE 'USA:%201%') AND mi.info IS NOT NULL AND (mi.info LIKE 'Japan:%201%'

       OR mi.info LIKE 'USA:%201%') AND (mi.info LIKE 'Japan:%201%'

       OR mi.info LIKE 'USA:%201%') AND (mi.info LIKE 'Japan:%201%'

       OR mi.info LIKE 'USA:%201%') AND (mi.info LIKE 'Japan:%201%'

       OR mi.info LIKE 'USA:%201%') AND (mi.info LIKE 'Japan:%201%'

       OR mi.info LIKE 'USA:%201%') AND cn.name = 'DreamWorks Animation' AND cn.country_code ='[us]' AND cn.name = 'DreamWorks Animation' AND cn.name = 'DreamWorks Animation' AND cn.name = 'DreamWorks Animation' AND cn.name = 'DreamWorks Animation' AND cn.name = 'DreamWorks Animation' AND k.keyword IN ('hero',

                    'martial-arts',

                    'hand-to-hand-combat',

                    'computer-animated-movie') AND n.gender ='f' AND n.name LIKE '%An%' AND ci.note IN ('(voice)',

                  '(voice: Japanese version)',

                  '(voice) (uncredited)',

                  '(voice: English version)') AND it.info = 'release dates' AND t.title LIKE 'Kung Fu Panda%' AND t.production_year > 2010 AND rt.role ='actress' AND t.id = mi.movie_id AND n.id = an.person_id AND t.id = mc.movie_id AND ci.movie_id = mk.movie_id AND mc.movie_id = mk.movie_id AND rt.id = ci.role_id AND mi.movie_id = ci.movie_id AND t.id = mk.movie_id AND mc.movie_id = mi.movie_id AND ci.person_id = an.person_id AND it.id = mi.info_type_id AND cn.id = mc.company_id AND n.id = ci.person_id AND k.id = mk.keyword_id AND t.id = ci.movie_id AND chn.id = ci.person_role_id AND mc.movie_id = ci.movie_id AND mi.movie_id = mk.movie_id;