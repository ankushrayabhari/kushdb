SELECT MIN(chn.name) AS chracter,

       MIN(t.title) AS movie_with_american_producer

FROM char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     company_type AS ct,

     movie_companies AS mc,

     role_type AS rt,

     title AS t WHERE mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND cn.country_code = '[us]' AND cn.country_code = '[us]' AND cn.country_code = '[us]' AND cn.country_code = '[us]' AND cn.country_code = '[us]' AND cn.country_code = '[us]' AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND chn.id >= 0 AND rt.id >= 0 AND rt.id >= 0 AND rt.id >= 0 AND rt.id >= 0 AND rt.id >= 0 AND t.production_year > 1990 AND ci.note LIKE '%(producer)%' AND ct.id = mc.company_type_id AND t.id = mc.movie_id AND rt.id = ci.role_id AND ci.movie_id = mc.movie_id AND cn.id = mc.company_id AND t.id = ci.movie_id AND chn.id = ci.person_role_id;