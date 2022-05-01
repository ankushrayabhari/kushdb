#pragma once

#include "magic_enum.hpp"

namespace kush::catalog {

enum class SqlType { SMALLINT, INT, BIGINT, REAL, DATE, TEXT, BOOLEAN };

struct Type {
  SqlType type_id;
  int32_t enum_id;

  std::string_view ToString() const { return magic_enum::enum_name(type_id); }

  bool operator==(const Type& rhs) const {
    return type_id == rhs.type_id && enum_id == rhs.enum_id;
  }

  bool operator!=(const Type& rhs) const { return !(*this == rhs); }

  static Type SmallInt() {
    return Type{
        .type_id = SqlType::SMALLINT,
        .enum_id = 0,
    };
  }

  static Type Int() {
    return Type{
        .type_id = SqlType::INT,
        .enum_id = 0,
    };
  }

  static Type BigInt() {
    return Type{
        .type_id = SqlType::BIGINT,
        .enum_id = 0,
    };
  }

  static Type Real() {
    return Type{
        .type_id = SqlType::REAL,
        .enum_id = 0,
    };
  }

  static Type Date() {
    return Type{
        .type_id = SqlType::DATE,
        .enum_id = 0,
    };
  }

  static Type Text() {
    return Type{
        .type_id = SqlType::TEXT,
        .enum_id = 0,
    };
  }

  static Type Boolean() {
    return Type{
        .type_id = SqlType::BOOLEAN,
        .enum_id = 0,
    };
  }
};

}  // namespace kush::catalog