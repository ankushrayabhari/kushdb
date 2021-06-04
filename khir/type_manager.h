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

class TypeTranslator {
 public:
  virtual void TranslateVoidType() = 0;
  virtual void TranslateI1Type() = 0;
  virtual void TranslateI8Type() = 0;
  virtual void TranslateI16Type() = 0;
  virtual void TranslateI32Type() = 0;
  virtual void TranslateI64Type() = 0;
  virtual void TranslateF64Type() = 0;
  virtual void TranslatePointerType(Type elem) = 0;
  virtual void TranslateArrayType(Type elem, int len) = 0;
  virtual void TranslateFunctionType(Type result,
                                     absl::Span<const Type> arg_types) = 0;
  virtual void TranslateStructType(absl::Span<const Type> elem_types) = 0;
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
  Type GetFunctionReturnType(Type func_type) const;
  Type GetPointerElementType(Type ptr_type) const;
  std::pair<int64_t, Type> GetPointerOffset(Type t,
                                            absl::Span<const int32_t> idx);
  std::vector<uint64_t> GetStructFieldOffsets(Type t) const;
  uint64_t GetTypeSize(Type t) const;

  void Translate(TypeTranslator& translator) const;

  bool IsVoid(Type t) const;
  bool IsF64Type(Type t) const;
  bool IsI1Type(Type t) const;
  bool IsI8Type(Type t) const;
  bool IsI16Type(Type t) const;
  bool IsI32Type(Type t) const;
  bool IsI64Type(Type t) const;
  bool IsPtrType(Type t) const;

 private:
  class TypeImpl {
   public:
    virtual ~TypeImpl() = default;
    virtual llvm::Type* GetLLVM() = 0;
    virtual Type Get() = 0;
  };

  enum BaseTypeId { VOID, I1, I8, I16, I32, I64, F64 };

  class BaseTypeImpl : public TypeImpl {
   public:
    BaseTypeImpl(BaseTypeId id, Type type, llvm::Type* type_impl);
    virtual ~BaseTypeImpl() = default;

    llvm::Type* GetLLVM() override;
    Type Get() override;
    BaseTypeId TypeId();

   private:
    BaseTypeId id_;
    Type type_;
    llvm::Type* type_impl_;
  };

  class PointerTypeImpl : public TypeImpl {
   public:
    PointerTypeImpl(Type elem, Type type, llvm::Type* type_impl);
    virtual ~PointerTypeImpl() = default;

    llvm::Type* GetLLVM() override;
    Type Get() override;
    Type ElementType();

   private:
    Type elem_;
    Type type_;
    llvm::Type* type_impl_;
  };

  class ArrayTypeImpl : public TypeImpl {
   public:
    ArrayTypeImpl(Type elem, int len, Type type, llvm::Type* type_impl);
    virtual ~ArrayTypeImpl() = default;

    llvm::Type* GetLLVM() override;
    Type Get() override;
    Type ElementType();
    int Length();

   private:
    Type elem_;
    int len_;
    Type type_;
    llvm::Type* type_impl_;
  };

  class FunctionTypeImpl : public TypeImpl {
   public:
    FunctionTypeImpl(Type result, absl::Span<const Type> arg_types, Type type,
                     llvm::Type* type_impl);
    virtual ~FunctionTypeImpl() = default;

    llvm::Type* GetLLVM() override;
    Type Get() override;
    Type ResultType();
    absl::Span<const Type> ArgTypes();

   private:
    Type result_type_;
    std::vector<Type> arg_types_;
    Type type_;
    llvm::Type* type_impl_;
  };

  class StructTypeImpl : public TypeImpl {
   public:
    StructTypeImpl(absl::Span<const Type> element_types, Type type,
                   llvm::Type* type_impl);
    virtual ~StructTypeImpl() = default;

    llvm::Type* GetLLVM() override;
    Type Get() override;
    absl::Span<const Type> ElementTypes();

   private:
    std::vector<Type> elem_types_;
    Type type_;
    llvm::Type* type_impl_;
  };

  std::vector<llvm::Type*> GetTypeArray(
      std::vector<std::unique_ptr<TypeImpl>>& types,
      absl::Span<const Type> field_type_id);

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  std::vector<std::unique_ptr<TypeImpl>> type_id_to_impl_;
  absl::flat_hash_map<std::string, Type> struct_name_to_type_id_;
};

}  // namespace kush::khir
