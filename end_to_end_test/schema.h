#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    /*
      CREATE TABLE people (
        id INTEGER NOT NULL,
        name TEXT NOT NULL,
      );
    */
    auto& table = db.Insert("people");
    table.Insert("id", SqlType::INT, "end_to_end_test/data/people_id.kdb");
    table.Insert("name", SqlType::TEXT, "end_to_end_test/data/people_name.kdb");
  }

  {
    /*
      CREATE TABLE info (
        id INTEGER NOT NULL,
        cheated BOOLEAN NOT NULL,
        date TIMESTAMP NOT NULL,
        zscore REAL NOT NULL,
        num1 SMALLINT NOT NULL,
        num2 BIGINT NOT NULL,
      );
    */
    auto& table = db.Insert("info");
    table.Insert("id", SqlType::INT, "end_to_end_test/data/info_id.kdb");
    table.Insert("cheated", SqlType::BOOLEAN,
                 "end_to_end_test/data/info_cheated.kdb");
    table.Insert("date", SqlType::DATE, "end_to_end_test/data/info_date.kdb");
    table.Insert("zscore", SqlType::REAL,
                 "end_to_end_test/data/info_zscore.kdb");
    table.Insert("num1", SqlType::SMALLINT,
                 "end_to_end_test/data/info_num1.kdb");
    table.Insert("num2", SqlType::BIGINT, "end_to_end_test/data/info_num2.kdb");
  }

  return db;
}