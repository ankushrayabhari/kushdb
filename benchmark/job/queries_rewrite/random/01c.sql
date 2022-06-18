SELECT MIN(mc.note) AS production_note,

       MIN(t.title) AS movie_title,

       MIN(t.production_year) AS movie_year

FROM company_type AS ct,

     info_type AS it,

     movie_companies AS mc,

     movie_info_idx AS mi_idx,

     title AS t WHERE t.production_year >2010 AND t.production_year >2010 AND t.production_year >2010 AND t.production_year >2010 AND t.production_year >2010 AND t.production_year >2010 AND (mc.note LIKE '%(co-production)%') AND mc.note NOT LIKE '%(as Metro-Goldwyn-Mayer Pictures)%' AND (mc.note LIKE '%(co-production)%') AND (mc.note LIKE '%(co-production)%') AND (mc.note LIKE '%(co-production)%') AND (mc.note LIKE '%(co-production)%') AND (mc.note LIKE '%(co-production)%') AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND ct.kind = 'production companies' AND it.info = 'top 250 rank' AND it.info = 'top 250 rank' AND it.info = 'top 250 rank' AND it.info = 'top 250 rank' AND it.info = 'top 250 rank' AND it.info = 'top 250 rank' AND ct.id = mc.company_type_id AND t.id = mc.movie_id AND it.id = mi_idx.info_type_id AND t.id = mi_idx.movie_id AND mc.movie_id = mi_idx.movie_id;