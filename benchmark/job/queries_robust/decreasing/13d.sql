SELECT MIN(cn.name) AS producing_company,

       MIN(miidx.info) AS rating,

       MIN(t.title) AS movie

FROM company_name AS cn,

     company_type AS ct,

     info_type AS it,

     info_type AS it2,

     kind_type AS kt,

     movie_companies AS mc,

     movie_info AS mi,

     movie_info_idx AS miidx,

     title AS t WHERE mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND mi.info_type_id >= 0 AND mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND mc.company_type_id >= 0 AND t.kind_id >= 0 AND t.kind_id >= 0 AND t.kind_id >= 0 AND t.kind_id >= 0 AND t.kind_id >= 0 AND miidx.movie_id >= 0 AND miidx.movie_id >= 0 AND miidx.movie_id >= 0 AND miidx.movie_id >= 0 AND miidx.movie_id >= 0 AND cn.country_code ='[us]' AND ct.kind ='production companies' AND it.info ='rating' AND it2.info ='release dates' AND kt.kind ='movie' AND ct.id = mc.company_type_id AND mi.movie_id = mc.movie_id AND it.id = miidx.info_type_id AND kt.id = t.kind_id AND mc.movie_id = t.id AND mi.movie_id = miidx.movie_id AND miidx.movie_id = mc.movie_id AND miidx.movie_id = t.id AND cn.id = mc.company_id AND it2.id = mi.info_type_id AND mi.movie_id = t.id;