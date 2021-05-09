#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

namespace kush::khir {

class TypeManager {
 public:
  TypeManager();

  int16_t VoidType();
  int16_t I1Type();
  int16_t I8Type();
  int16_t I16Type();
  int16_t I32Type();
  int16_t I64Type();
  int16_t F64Type();
  int16_t NamedStructType(absl::Span<const int16_t> field_type_id,
                          std::string_view name);
  int16_t StructType(absl::Span<const int16_t> field_type_id);

  int16_t PointerType(int16_t type);
  int16_t ArrayType(int16_t type, int len);
  int16_t FunctionType(int16_t result, absl::Span<const int16_t> args);

  int16_t GetNamedStructType(std::string_view name);

 private:
  class PointerTypeImpl {
   public:
    PointerTypeImpl(int16_t element_type_id);

    int16_t ElementType();

   private:
    int16_t element_type_;
  };

  class ArrayTypeImpl {
   public:
    ArrayTypeImpl(int16_t element_type_id, int length);

    int16_t ElementType();
    int Length();

   private:
    int16_t element_type_;
    int length_;
  };

  class FunctionTypeImpl {
   public:
    FunctionTypeImpl(int16_t result_type_id,
                     absl::Span<const int16_t> arg_type_id);

    int16_t ResultType();
    absl::Span<const int16_t> ArgTypes();

   private:
    int16_t result_type_;
    std::vector<int16_t> arg_type_ids_;
  };

  class StructTypeImpl {
   public:
    StructTypeImpl(absl::Span<const int16_t> field_type_ids);

    absl::Span<const int16_t> FieldTypes();

   private:
    std::vector<int16_t> field_type_ids_;
  };

  enum class BaseTypeImpl { VOID, I1, I8, I16, I32, I64, F64 };

  std::vector<std::variant<BaseTypeImpl, PointerTypeImpl, ArrayTypeImpl,
                           FunctionTypeImpl, StructTypeImpl>>
      type_id_to_impl_;
  absl::flat_hash_map<std::string, int16_t> struct_name_to_type_id_;
};

}  // namespace kush::khir
