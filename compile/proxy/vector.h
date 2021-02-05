#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/type.h"

namespace kush::compile::proxy {

template <typename T>
class StructBuilder {
 public:
  StructBuilder(ProgramBuilder<T>& program);
  void Add(proxy::TypeID type);
  typename ProgramBuilder<T>::Type& Build();
};

template <typename T>
class Struct {
 public:
  Struct(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);

  void Pack(std::vector<std::reference_wrapper<const proxy::Value<T>>> value);

  std::vector<std::unique_ptr<Value<T>>> Unpack();
};

template <typename T>
class Vector {
 public:
  Vector(ProgramBuilder<T>& program, StructBuilder<T>& content)
      : program_(program) {
    content_type = &content.Build();
    struct_type = &program_.StructType({
        program_.UI32Type(),               // capacity
        program_.UI32Type(),               // size
        program_.ArrayType(*content_type)  // data
    });
    struct_ptr_type = &program_.PointerType(*struct_type);

    auto two = proxy::UInt32<T>(program_, 2);

    // create the struct of initial capacity 2
    data = &Create(two);

    // capacity = 2, size = 0
    CapacityPtr(data).Store(proxy::UInt32<T>(program_, 2));
    SizePtr(data).Store(proxy::UInt32<T>(program_, 0));
  }

  ~Vector() { program_.Free(*data); }

  Struct<T> Append() {
    auto capacity = CapacityPtr(data).Load();
    auto size = SizePtr(data).Load();

    // Double when at capacity
    typename ProgramBuilder<T>::Value* new_data;
    proxy::If<T> check(program_, capacity >= size, [&]() {
      auto new_capacity = capacity * proxy::UInt32<T>(program_, 2);
      new_data = &Create(new_capacity);

      CapacityPtr(new_data).Store(new_capacity);
      SizePtr(new_data).Store(size);

      Copy(data, new_data, size);
    });

    data = &check.Phi(*data, *new_data);

    auto next = Get(size);
    SizePtr(data).Store(size + proxy::UInt32<T>(program_, 1));
    return std::move(next);
  }

  void Sort() {}

  UInt32<T> Size() { return SizePtr(data).Load(); }

  Struct<T> Get(const proxy::UInt32<T>& idx) {
    return Struct<T>(
        program_, program_.GetElementPtr(
                      *struct_ptr_type, *data,
                      {program_.ConstI32(0), program_.ConstI32(2), idx.Get()}));
  }

 private:
  typename ProgramBuilder<T>::Value& Create(
      const proxy::UInt32<T>& new_capacity) {
    auto& size_ptr = program_.GetElementPtr(
        *struct_ptr_type, program_.NullPtr(),
        {program_.ConstI32(0), program_.ConstI32(2), new_capacity.Get()});
    auto& size = program_.PointerCast(size_ptr, program_.UI32Type());
    auto& ptr = program_.Malloc(size);
    return program_.PointerCast(ptr, *struct_ptr_type);
  }

  void Copy(typename ProgramBuilder<T>::Value* source,
            typename ProgramBuilder<T>::Value* dest,
            const proxy::UInt32<T>& size) {
    auto& old_data_ptr =
        program_.GetElementPtr(*struct_ptr_type, *source,
                               {program_.ConstI32(0), program_.ConstI32(2)});
    auto& length_ptr = program_.GetElementPtr(
        *struct_ptr_type, program_.NullPtr(),
        {program_.ConstI32(0), program_.ConstI32(2), size.Get()});
    auto& length = program_.PointerCast(length_ptr, program_.UI32Type());
    auto& new_data_ptr = program_.GetElementPtr(
        *struct_ptr_type, *dest, {program_.ConstI32(0), program_.ConstI32(2)});
    program_.Memcpy(program_.PointerCast(new_data_ptr, program_.I8Type()),
                    program_.PointerCast(old_data_ptr, program_.I8Type()),
                    length);
  }

  Ptr<T, UInt32<T>> CapacityPtr(typename ProgramBuilder<T>::Value* target) {
    return Ptr<T, UInt32<T>>(
        program_,
        program_.GetElementPtr(*struct_ptr_type, *target,
                               {program_.ConstI32(0), program_.ConstI32(0)}));
  }

  Ptr<T, UInt32<T>> SizePtr(typename ProgramBuilder<T>::Value* target) {
    return Ptr<T, UInt32<T>>(
        program_,
        program_.GetElementPtr(*struct_ptr_type, *target,
                               {program_.ConstI32(0), program_.ConstI32(1)}));
  }

  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Type* struct_type;
  typename ProgramBuilder<T>::Type* struct_ptr_type;
  typename ProgramBuilder<T>::Type* content_type;
  typename ProgramBuilder<T>::Value* data;
};

}  // namespace kush::compile::proxy