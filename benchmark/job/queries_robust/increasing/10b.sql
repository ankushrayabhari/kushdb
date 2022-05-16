SELECT MIN(chn.name) AS chracter,

       MIN(t.title) AS russian_mov_with_actor_producer

FROM char_name AS chn,

     cast_info AS ci,

     company_name AS cn,

     company_type AS ct,

     movie_companies AS mc,

     role_type AS rt,

     title AS t WHERE rt.role = 'actor' AND rt.role = 'actor' AND rt.role = 'actor' AND rt.role = 'actor' AND rt.role = 'actor' AND rt.role = 'actor' AND ct.id >= 0 AND ct.id >= 0 AND ct.id >= 0 AND ct.id >= 0 AND ct.id >= 0 AND cn.country_code = '[ru]' AND cn.country_code = '[ru]' AND cn.country_code = '[ru]' AND cn.country_code = '[ru]' AND cn.country_code = '[ru]' AND cn.country_code = '[ru]' AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND t.production_year > 2010 AND ci.note LIKE '%(producer)%' AND ct.id = mc.company_type_id AND t.id = mc.movie_id AND rt.id = ci.role_id AND ci.movie_id = mc.movie_id AND cn.id = mc.company_id AND t.id = ci.movie_id AND chn.id = ci.person_role_id;