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

  uint16_t VoidType();
  uint16_t I1Type();
  uint16_t I8Type();
  uint16_t I16Type();
  uint16_t I32Type();
  uint16_t I64Type();
  uint16_t F64Type();
  uint16_t NamedStructType(absl::Span<const uint16_t> field_type_id,
                           std::string_view name);
  uint16_t StructType(absl::Span<const uint16_t> field_type_id);

  uint16_t PointerType(uint16_t type);
  uint16_t ArrayType(uint16_t type, int len);
  uint16_t FunctionType(uint16_t result, absl::Span<const uint16_t> args);

  uint16_t GetNamedStructType(std::string_view name);

 private:
  class PointerTypeImpl {
   public:
    PointerTypeImpl(uint16_t element_type_id);

    uint16_t ElementType();

   private:
    uint16_t element_type_;
  };

  class ArrayTypeImpl {
   public:
    ArrayTypeImpl(uint16_t element_type_id, int length);

    uint16_t ElementType();
    int Length();

   private:
    uint16_t element_type_;
    int length_;
  };

  class FunctionTypeImpl {
   public:
    FunctionTypeImpl(uint16_t result_type_id,
                     absl::Span<const uint16_t> arg_type_id);

    uint16_t ResultType();
    absl::Span<const uint16_t> ArgTypes();

   private:
    uint16_t result_type_;
    std::vector<uint16_t> arg_type_ids_;
  };

  class StructTypeImpl {
   public:
    StructTypeImpl(absl::Span<const uint16_t> field_type_ids);

    absl::Span<const uint16_t> FieldTypes();

   private:
    std::vector<uint16_t> field_type_ids_;
  };

  enum class BaseTypeImpl { VOID, I1, I8, I16, I32, I64, F64 };

  std::vector<std::variant<BaseTypeImpl, PointerTypeImpl, ArrayTypeImpl,
                           FunctionTypeImpl, StructTypeImpl>>
      type_id_to_impl_;
  absl::flat_hash_map<std::string, uint16_t> struct_name_to_type_id_;
};

}  // namespace kush::khir
