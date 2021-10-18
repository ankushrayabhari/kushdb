CREATE INDEX aka_name_person_id ON aka_name(person_id);
CREATE INDEX aka_name_name ON aka_name(name);
CREATE INDEX aka_name_imdb_index ON aka_name(imdb_index);
CREATE INDEX aka_name_name_pcode_cf ON aka_name(name_pcode_cf);
CREATE INDEX aka_name_name_pcode_nf ON aka_name(name_pcode_nf);
CREATE INDEX aka_name_surname_pcode ON aka_name(surname_pcode);
CREATE INDEX aka_name_md5sum ON aka_name(md5sum);

CREATE INDEX aka_title_movie_id ON aka_title(movie_id);
CREATE INDEX aka_title_title ON aka_title(title);
CREATE INDEX aka_title_imdb_index ON aka_title(imdb_index);
CREATE INDEX aka_title_kind_id ON aka_title(kind_id);
CREATE INDEX aka_title_production_year ON aka_title(production_year);
CREATE INDEX aka_title_phonetic_code ON aka_title(phonetic_code);
CREATE INDEX aka_title_episode_of_id ON aka_title(episode_of_id);
CREATE INDEX aka_title_season_nr ON aka_title(season_nr);
CREATE INDEX aka_title_episode_nr ON aka_title(episode_nr);
CREATE INDEX aka_title_note ON aka_title(note);
CREATE INDEX aka_title_md5sum ON aka_title(md5sum);

CREATE INDEX cast_info_person_id ON cast_info(person_id);
CREATE INDEX cast_info_movie_id ON cast_info(movie_id);
CREATE INDEX cast_info_person_role_id ON cast_info(person_role_id);
CREATE INDEX cast_info_note ON cast_info(note);
CREATE INDEX cast_info_nr_order ON cast_info(nr_order);
CREATE INDEX cast_info_role_id ON cast_info(role_id);

CREATE INDEX char_name_name ON char_name(name);
CREATE INDEX char_name_imdb_index ON char_name(imdb_index);
CREATE INDEX char_name_imdb_id ON char_name(imdb_id);
CREATE INDEX char_name_name_pcode_nf ON char_name(name_pcode_nf);
CREATE INDEX char_name_surname_pcode ON char_name(surname_pcode);
CREATE INDEX char_name_md5sum ON char_name(md5sum);

CREATE INDEX comp_cast_type_kind ON comp_cast_type(kind);

CREATE INDEX company_name_name ON company_name(name);
CREATE INDEX company_name_country_code ON company_name(country_code);
CREATE INDEX company_name_imdb_id ON company_name(imdb_id);
CREATE INDEX company_name_name_pcode_nf ON company_name(name_pcode_nf);
CREATE INDEX company_name_name_pcode_sf ON company_name(name_pcode_sf);
CREATE INDEX company_name_md5sum ON company_name(md5sum);

CREATE INDEX company_type_kind ON company_type(kind);

CREATE INDEX complete_cast_movie_id ON complete_cast(movie_id);
CREATE INDEX complete_cast_subject_id ON complete_cast(subject_id);
CREATE INDEX complete_cast_status_id ON complete_cast(status_id);

CREATE INDEX info_type_info ON info_type(info);

CREATE INDEX keyword_keyword ON keyword(keyword);
CREATE INDEX keyword_phonetic_code ON keyword(phonetic_code);

CREATE INDEX kind_type_kind ON kind_type(kind);

CREATE INDEX link_type_link ON link_type(link);

CREATE INDEX movie_companies_movie_id ON movie_companies(movie_id);
CREATE INDEX movie_companies_company_id ON movie_companies(company_id);
CREATE INDEX movie_companies_company_type_id ON movie_companies(company_type_id);
CREATE INDEX movie_companies_note ON movie_companies(note);

CREATE INDEX movie_info_movie_id ON movie_info(movie_id);
CREATE INDEX movie_info_info_type_id ON movie_info(info_type_id);
CREATE INDEX movie_info_info ON movie_info(info);
CREATE INDEX movie_info_note ON movie_info(note);

CREATE INDEX movie_info_idx_movie_id ON movie_info_idx(movie_id);
CREATE INDEX movie_info_idx_info_type_id ON movie_info_idx(info_type_id);
CREATE INDEX movie_info_idx_info ON movie_info_idx(info);
CREATE INDEX movie_info_idx_note ON movie_info_idx(note);

CREATE INDEX movie_keyword_movie_id ON movie_keyword(movie_id);
CREATE INDEX movie_keyword_keyword_id ON movie_keyword(keyword_id);

CREATE INDEX movie_link_movie_id ON movie_link(movie_id);
CREATE INDEX movie_link_linked_movie_id ON movie_link(linked_movie_id);
CREATE INDEX movie_link_link_type_id ON movie_link(link_type_id);

CREATE INDEX name_name ON name(name);
CREATE INDEX name_imdb_index ON name(imdb_index);
CREATE INDEX name_imdb_id ON name(imdb_id);
CREATE INDEX name_gender ON name(gender);
CREATE INDEX name_name_pcode_cf ON name(name_pcode_cf);
CREATE INDEX name_name_pcode_nf ON name(name_pcode_nf);
CREATE INDEX name_surname_pcode ON name(surname_pcode);
CREATE INDEX name_md5sum ON name(md5sum);

CREATE INDEX person_info_person_id ON person_info(person_id);
CREATE INDEX person_info_info_type_id ON person_info(info_type_id);
CREATE INDEX person_info_info ON person_info(info);
CREATE INDEX person_info_note ON person_info(note);

CREATE INDEX role_type_role ON role_type(role);

CREATE INDEX title_title ON title(title);
CREATE INDEX title_imdb_index ON title(imdb_index);
CREATE INDEX title_kind_id ON title(kind_id);
CREATE INDEX title_production_year ON title(production_year);
CREATE INDEX title_imdb_id ON title(imdb_id);
CREATE INDEX title_phonetic_code ON title(phonetic_code);
CREATE INDEX title_episode_of_id ON title(episode_of_id);
CREATE INDEX title_season_nr ON title(season_nr);
CREATE INDEX title_episode_nr ON title(episode_nr);
CREATE INDEX title_series_years ON title(series_years);
CREATE INDEX title_md5sum ON title(md5sum);