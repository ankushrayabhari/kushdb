SELECT MIN(mi.info) AS movie_budget,

       MIN(mi_idx.info) AS movie_votes,

       MIN(t.title) AS movie_title

FROM cast_info AS ci,

     info_type AS it1,

     info_type AS it2,

     movie_info AS mi,

     movie_info_idx AS mi_idx,

     name AS n,

     title AS t WHERE it1.info = 'genres' AND it1.info = 'genres' AND it1.info = 'genres' AND it1.info = 'genres' AND it1.info = 'genres' AND it1.info = 'genres' AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND n.gender = 'm' AND ci.note IN ('(writer)',

                  '(head writer)',

                  '(written by)',

                  '(story)',

                  '(story editor)') AND ci.note IN ('(writer)',

                  '(head writer)',

                  '(written by)',

                  '(story)',

                  '(story editor)') AND ci.note IN ('(writer)',

                  '(head writer)',

                  '(written by)',

                  '(story)',

                  '(story editor)') AND ci.note IN ('(writer)',

                  '(head writer)',

                  '(written by)',

                  '(story)',

                  '(story editor)') AND ci.note IN ('(writer)',

                  '(head writer)',

                  '(written by)',

                  '(story)',

                  '(story editor)') AND ci.note IN ('(writer)',

                  '(head writer)',

                  '(written by)',

                  '(story)',

                  '(story editor)') AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND t.id >= 0 AND it2.info = 'votes' AND mi.info IN ('Horror',

                  'Action',

                  'Sci-Fi',

                  'Thriller',

                  'Crime',

                  'War') AND ci.movie_id = mi_idx.movie_id AND t.id = mi.movie_id AND mi.movie_id = mi_idx.movie_id AND it2.id = mi_idx.info_type_id AND ci.movie_id = mi.movie_id AND it1.id = mi.info_type_id AND t.id = mi_idx.movie_id AND n.id = ci.person_id AND t.id = ci.movie_id;