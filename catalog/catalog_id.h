#pragma once

#include "type_safe/strong_typedef.hpp"

namespace kush::catalog {

struct TableId : type_safe::strong_typedef<TableId, int>,
                 type_safe::strong_typedef_op::equality_comparison<TableId> {
  using strong_typedef::strong_typedef;
};

struct ColumnId : type_safe::strong_typedef<ColumnId, int>,
                  type_safe::strong_typedef_op::equality_comparison<ColumnId> {
  using strong_typedef::strong_typedef;
};

}  // namespace kush::catalog