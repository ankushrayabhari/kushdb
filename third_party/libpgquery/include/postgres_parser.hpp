//===----------------------------------------------------------------------===//
//                         DuckDB
//
// postgres_parser.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <vector>

#include "nodes/pg_list.hpp"
#include "pg_simplified_token.hpp"

namespace libpgquery {
class PostgresParser {
 public:
  PostgresParser();
  ~PostgresParser();

  bool success;
  libpgquery::PGList *parse_tree;
  std::string error_message;
  int error_location;

 public:
  void Parse(const std::string &query);
  static std::vector<libpgquery::PGSimplifiedToken> Tokenize(
      const std::string &query);

  static bool IsKeyword(const std::string &text);
};
}  // namespace libpgquery
