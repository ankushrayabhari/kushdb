#include "duckdb_libpgquery/parser.h"

#include <stdexcept>
#include <string>

int main() {
  std::string query = "SELECT * FROM lineitem;";

  duckdb_libpgquery::PostgresParser parser;
  parser.Parse(query);

  if (!parser.success) {
    throw std::runtime_error(parser.error_message + ": " + query);
  }
}