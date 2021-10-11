#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    /*
      CREATE TABLE people (
        id INTEGER,
        name TEXT,
      );
    */
    auto& table = db.Insert("people");
    table.Insert("id", SqlType::INT, "end_to_end_test/data/people_id.kdb",
                 "end_to_end_test/data/people_id_null.kdb",
                 "end_to_end_test/data/people_id.kdbindex");
    table.Insert("name", SqlType::TEXT, "end_to_end_test/data/people_name.kdb",
                 "end_to_end_test/data/people_name_null.kdb",
                 "end_to_end_test/data/people_name.kdbindex");
  }

  {
    /*
      CREATE TABLE info (
        id INTEGER,
        cheated BOOLEAN,
        date TIMESTAMP,
        zscore REAL,
        num1 SMALLINT,
        num2 BIGINT,
      );
    */
    auto& table = db.Insert("info");
    table.Insert("id", SqlType::INT, "end_to_end_test/data/info_id.kdb",
                 "end_to_end_test/data/info_id_null.kdb",
                 "end_to_end_test/data/info_id.kdbindex");
    table.Insert("cheated", SqlType::BOOLEAN,
                 "end_to_end_test/data/info_cheated.kdb",
                 "end_to_end_test/data/info_cheated_null.kdb",
                 "end_to_end_test/data/info_cheated.kdbindex");
    table.Insert("date", SqlType::DATE, "end_to_end_test/data/info_date.kdb",
                 "end_to_end_test/data/info_date_null.kdb",
                 "end_to_end_test/data/info_date.kdbindex");
    table.Insert("zscore", SqlType::REAL,
                 "end_to_end_test/data/info_zscore.kdb",
                 "end_to_end_test/data/info_zscore_null.kdb",
                 "end_to_end_test/data/info_zscore.kdbindex");
    table.Insert("num1", SqlType::SMALLINT,
                 "end_to_end_test/data/info_num1.kdb",
                 "end_to_end_test/data/info_num1_null.kdb",
                 "end_to_end_test/data/info_num1.kdbindex");
    table.Insert("num2", SqlType::BIGINT, "end_to_end_test/data/info_num2.kdb",
                 "end_to_end_test/data/info_num2_null.kdb",
                 "end_to_end_test/data/info_num2.kdbindex");
  }

  return db;
}