#include "parse/transform/transform_alias.h"

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::string TransformAlias(duckdb_libpgquery::PGAlias* root) {
  if (!root) {
    return "";
  }

  if (root->colnames) {
    throw std::runtime_error("Column aliases not supported.");
  }

  return root->aliasname;
}

}  // namespace kush::parse