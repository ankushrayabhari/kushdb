SELECT MIN(mi.info) AS movie_budget,

       MIN(mi_idx.info) AS movie_votes,

       MIN(n.name) AS writer,

       MIN(t.title) AS complete_violent_movie

FROM complete_cast AS cc,

     comp_cast_type AS cct1,

     comp_cast_type AS cct2,

     cast_info AS ci,

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

                  '(story editor)') AND mi.info IN ('Horror',

                  'Action',

                  'Sci-Fi',

                  'Thriller',

                  'Crime',

                  'War') AND k.keyword IN ('murder',

                    'violence',

                    'blood',

                    'gore',

                    'death',

                    'female-nudity',

                    'hospital') AND cct1.kind = 'cast' AND cct2.kind ='complete+verified' AND it1.info = 'genres' AND it2.info = 'votes' AND t.id = cc.movie_id AND ci.movie_id = mk.movie_id AND mk.movie_id = cc.movie_id AND cct1.id = cc.subject_id AND mi.movie_id = cc.movie_id AND ci.movie_id = mi_idx.movie_id AND it2.id = mi_idx.info_type_id AND mi.movie_id = mk.movie_id AND ci.movie_id = cc.movie_id AND ci.movie_id = mi.movie_id AND mi_idx.movie_id = mk.movie_id AND cct2.id = cc.status_id AND mi_idx.movie_id = cc.movie_id AND t.id = mi.movie_id AND mi.movie_id = mi_idx.movie_id AND t.id = mk.movie_id AND it1.id = mi.info_type_id AND t.id = mi_idx.movie_id AND n.id = ci.person_id AND k.id = mk.keyword_id AND t.id = ci.movie_id;