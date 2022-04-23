#include "parse/transform/transform_column_ref_expression.h"

#include <memory>
#include <vector>

#include "parse/expression/column_ref_expression.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<ColumnRefExpression> TransformColumnRefExpression(
    libpgquery::PGColumnRef& colref) {
  auto fields = colref.fields;
  auto head_node = (libpgquery::PGNode*)fields->head->data.ptr_value;
  switch (head_node->type) {
    case libpgquery::T_PGString: {
      if (fields->length < 1) {
        throw std::runtime_error("Unexpected field length");
      }

      if (fields->length == 1) {
        /*
        std::string column_name(reinterpret_cast<libpgquery::PGValue*>(
                                    fields->head->data.ptr_value)
                                    ->val.str);
        return std::make_unique<ColumnRefExpression>(column_name);
        */
        throw std::runtime_error("Unqualified column ref unsupported.");
      }

      if (fields->length == 2) {
        std::string table_name(
            reinterpret_cast<libpgquery::PGValue*>(fields->head->data.ptr_value)
                ->val.str);
        auto col_node = reinterpret_cast<libpgquery::PGNode*>(
            fields->head->next->data.ptr_value);
        switch (col_node->type) {
          case libpgquery::T_PGString: {
            std::string column_name(
                reinterpret_cast<libpgquery::PGValue*>(col_node)->val.str);
            return std::make_unique<ColumnRefExpression>(column_name,
                                                         table_name);
          }

          default:
            throw std::runtime_error("ColumnRef not supported.");
        }
      }

      throw std::runtime_error("ColumnRef not supported.");
    }

    case libpgquery::T_PGAStar: {
      throw std::runtime_error("Star not supported.");
    }

    default:
      throw std::runtime_error("ColumnRef not supported.");
  }
}

}  // namespace kush::parse