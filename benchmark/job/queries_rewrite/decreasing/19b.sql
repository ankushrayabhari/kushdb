SELECT MIN(n.name) AS voicing_actress,

       MIN(t.title) AS kung_fu_panda

FROM aka_name AS an,

     char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     info_type AS it,

     movie_companies AS mc,

     movie_info AS mi,

     name AS n,

     role_type AS rt,

     title AS t WHERE chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND an.person_id >= 0 AND ci.note = '(voice)' AND ci.note = '(voice)' AND ci.note = '(voice)' AND ci.note = '(voice)' AND ci.note = '(voice)' AND ci.note = '(voice)' AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND mc.note LIKE '%(200%)%' AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND (mc.note LIKE '%(USA)%'

       OR mc.note LIKE '%(worldwide)%') AND cn.country_code ='[us]' AND (mi.info LIKE 'Japan:%2007%'

       OR mi.info LIKE 'USA:%2008%') AND mi.info IS NOT NULL AND n.name LIKE '%Angel%' AND n.gender ='f' AND t.title LIKE '%Kung%Fu%Panda%' AND t.production_year between 2007 and 2008 AND it.info = 'release dates' AND rt.role ='actress' AND t.id = mi.movie_id AND n.id = an.person_id AND t.id = mc.movie_id AND mi.movie_id = ci.movie_id AND rt.id = ci.role_id AND mc.movie_id = mi.movie_id AND ci.person_id = an.person_id AND it.id = mi.info_type_id AND cn.id = mc.company_id AND n.id = ci.person_id AND t.id = ci.movie_id AND chn.id = ci.person_role_id AND mc.movie_id = ci.movie_id;