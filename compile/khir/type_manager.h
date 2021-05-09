#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

namespace kush::khir {

struct Type : type_safe::strong_typedef<Type, uint16_t>,
              type_safe::strong_typedef_op::equality_comparison<Type> {
  using strong_typedef::strong_typedef;

  uint16_t GetID();
};

class TypeManager {
 public:
  TypeManager();

  Type VoidType();
  Type I1Type();
  Type I8Type();
  Type I16Type();
  Type I32Type();
  Type I64Type();
  Type F64Type();
  Type NamedStructType(absl::Span<const Type> field_type_id,
                       std::string_view name);
  Type StructType(absl::Span<const Type> field_type_id);

  Type PointerType(Type type);
  Type ArrayType(Type type, int len);
  Type FunctionType(Type result, absl::Span<const Type> args);

  Type GetNamedStructType(std::string_view name);

 private:
  class PointerTypeImpl {
   public:
    PointerTypeImpl(Type element_type_id);

    Type ElementType();

   private:
    Type element_type_;
  };

  class ArrayTypeImpl {
   public:
    ArrayTypeImpl(Type element_type_id, int length);

    Type ElementType();
    int Length();

   private:
    Type element_type_;
    int length_;
  };

  class FunctionTypeImpl {
   public:
    FunctionTypeImpl(Type result_type_id, absl::Span<const Type> arg_type_id);

    Type ResultType();
    absl::Span<const Type> ArgTypes();

   private:
    Type result_type_;
    std::vector<Type> arg_type_ids_;
  };

  class StructTypeImpl {
   public:
    StructTypeImpl(absl::Span<const Type> field_type_ids);

    absl::Span<const Type> FieldTypes();

   private:
    std::vector<Type> field_type_ids_;
  };

  enum class BaseTypeImpl { VOID, I1, I8, I16, I32, I64, F64 };

  std::vector<std::variant<BaseTypeImpl, PointerTypeImpl, ArrayTypeImpl,
                           FunctionTypeImpl, StructTypeImpl>>
      type_id_to_impl_;
  absl::flat_hash_map<std::string, Type> struct_name_to_type_id_;
};

}  // namespace kush::khir
