#include "postgres_parser.hpp"

#include "common/keywords.hpp"
#include "parser/parser.hpp"
#include "pg_functions.hpp"

using namespace std;

namespace libpgquery {

PostgresParser::PostgresParser()
    : success(false),
      parse_tree(nullptr),
      error_message(""),
      error_location(0) {}

void PostgresParser::Parse(const string &query) {
  libpgquery::pg_parser_init();
  libpgquery::parse_result res;
  pg_parser_parse(query.c_str(), &res);
  success = res.success;

  if (success) {
    parse_tree = res.parse_tree;
  } else {
    error_message = string(res.error_message);
    error_location = res.error_location;
  }
}

vector<libpgquery::PGSimplifiedToken> PostgresParser::Tokenize(
    const std::string &query) {
  libpgquery::pg_parser_init();
  auto tokens = libpgquery::tokenize(query.c_str());
  libpgquery::pg_parser_cleanup();
  return tokens;
}

PostgresParser::~PostgresParser() { libpgquery::pg_parser_cleanup(); }

bool PostgresParser::IsKeyword(const std::string &text) {
  return libpgquery::is_keyword(text.c_str());
}

}  // namespace libpgquery
