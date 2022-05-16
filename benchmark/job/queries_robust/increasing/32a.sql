SELECT MIN(lt.link) AS link_type,

       MIN(t1.title) AS first_movie,

       MIN(t2.title) AS second_movie

FROM keyword AS k,

     link_type AS lt,

     movie_keyword AS mk,

     movie_link AS ml,

     title AS t1,

     title AS t2 WHERE k.keyword ='10,000-mile-club' AND k.keyword ='10,000-mile-club' AND k.keyword ='10,000-mile-club' AND k.keyword ='10,000-mile-club' AND k.keyword ='10,000-mile-club' AND k.keyword ='10,000-mile-club' AND lt.id >= 0 AND lt.id >= 0 AND lt.id >= 0 AND lt.id >= 0 AND lt.id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND ml.link_type_id >= 0 AND t1.id >= 0 AND t1.id >= 0 AND t1.id >= 0 AND t1.id >= 0 AND t1.id >= 0 AND lt.id = ml.link_type_id AND t1.id = mk.movie_id AND ml.linked_movie_id = t2.id AND mk.movie_id = t1.id AND ml.movie_id = t1.id AND mk.keyword_id = k.id;