SELECT MIN(mi.info) AS movie_budget,

       MIN(mi_idx.info) AS movie_votes,

       MIN(n.name) AS male_writer,

       MIN(t.title) AS violent_movie_title

FROM cast_info AS ci,

     info_type AS it1,

     info_type AS it2,

     keyword AS k,

     movie_info AS mi,

     movie_info_idx AS mi_idx,

     movie_keyword AS mk,

     name AS n,

     title AS t WHERE mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND mk.keyword_id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND mi_idx.info_type_id >= 0 AND mi_idx.info_type_id >= 0 AND mi_idx.info_type_id >= 0 AND mi_idx.info_type_id >= 0 AND mi_idx.info_type_id >= 0 AND ci.note IN ('(writer)',

                  '(head writer)',

                  '(written by)',

                  '(story)',

                  '(story editor)') AND mi.info = 'Horror' AND k.keyword IN ('murder',

                    'blood',

                    'gore',

                    'death',

                    'female-nudity') AND it1.info = 'genres' AND it2.info = 'votes' AND ci.movie_id = mi_idx.movie_id AND t.id = mi.movie_id AND mi.movie_id = mi_idx.movie_id AND ci.movie_id = mk.movie_id AND it2.id = mi_idx.info_type_id AND t.id = mk.movie_id AND ci.movie_id = mi.movie_id AND it1.id = mi.info_type_id AND t.id = mi_idx.movie_id AND mi_idx.movie_id = mk.movie_id AND n.id = ci.person_id AND k.id = mk.keyword_id AND t.id = ci.movie_id AND mi.movie_id = mk.movie_id;