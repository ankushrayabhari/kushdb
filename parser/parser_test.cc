#include <stdexcept>
#include <string>

#include "third_party/duckdb_libpgquery/include/postgres_parser.hpp"

int main() {
  std::string query = "SELECT * FROM lineitem;";

  duckdb_libpgquery::PostgresParser parser;
  parser.Parse(query);

  if (!parser.success) {
    throw std::runtime_error(parser.error_message + ": " + query);
  }
}