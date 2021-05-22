#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace kush::khir {

struct Type : type_safe::strong_typedef<Type, uint16_t>,
              type_safe::strong_typedef_op::equality_comparison<Type> {
  using strong_typedef::strong_typedef;

  uint16_t GetID() const;
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
  Type GetFunctionReturnType(Type func_type);
  Type GetPointerElementType(Type ptr_type);
  std::pair<int64_t, Type> GetPointerOffset(Type t,
                                            absl::Span<const int32_t> idx);

 private:
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  std::vector<llvm::Type*> type_id_to_impl_;
  absl::flat_hash_map<std::string, Type> struct_name_to_type_id_;
};

}  // namespace kush::khir
