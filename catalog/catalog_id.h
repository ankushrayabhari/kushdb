#pragma once

#include "type_safe/strong_typedef.hpp"

namespace kush::catalog {

struct table_id_t
    : type_safe::strong_typedef<table_id_t, int>,
      type_safe::strong_typedef_op::equality_comparison<table_id_t> {
  using strong_typedef::strong_typedef;
};

struct column_id_t
    : type_safe::strong_typedef<column_id_t, int>,
      type_safe::strong_typedef_op::equality_comparison<column_id_t> {
  using strong_typedef::strong_typedef;
};

}  // namespace kush::catalog