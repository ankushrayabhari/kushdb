#pragma once

#include "magic_enum.hpp"

namespace kush::catalog {

enum class TypeId {
  SMALLINT,
  INT,
  BIGINT,
  REAL,
  DATE,
  TEXT,
  BOOLEAN,
  ENUM,
};

struct Type {
  TypeId type_id;
  int32_t enum_id;

  std::string_view ToString() const { return magic_enum::enum_name(type_id); }

  bool operator==(const Type& rhs) const {
    return type_id == rhs.type_id && enum_id == rhs.enum_id;
  }

  bool operator!=(const Type& rhs) const { return !(*this == rhs); }

  static Type SmallInt() {
    return Type{
        .type_id = TypeId::SMALLINT,
        .enum_id = 0,
    };
  }

  static Type Int() {
    return Type{
        .type_id = TypeId::INT,
        .enum_id = 0,
    };
  }

  static Type BigInt() {
    return Type{
        .type_id = TypeId::BIGINT,
        .enum_id = 0,
    };
  }

  static Type Real() {
    return Type{
        .type_id = TypeId::REAL,
        .enum_id = 0,
    };
  }

  static Type Date() {
    return Type{
        .type_id = TypeId::DATE,
        .enum_id = 0,
    };
  }

  static Type Text() {
    return Type{
        .type_id = TypeId::TEXT,
        .enum_id = 0,
    };
  }

  static Type Boolean() {
    return Type{
        .type_id = TypeId::BOOLEAN,
        .enum_id = 0,
    };
  }

  static Type Enum(int32_t id) {
    return Type{
        .type_id = TypeId::ENUM,
        .enum_id = id,
    };
  }
};

}  // namespace kush::catalog