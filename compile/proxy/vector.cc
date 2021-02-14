#include "compile/proxy/vector.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
Vector<T>::Vector(ProgramBuilder<T>& program, StructBuilder<T>& content)
    : program_(program), content_(content) {
  content_type = &content_.GenerateType();
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

template <typename T>
Vector<T>::~Vector() {
  program_.Free(*data);
}

template <typename T>
Struct<T> Vector<T>::Append() {
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

template <typename T>
void Vector<T>::Sort(typename ProgramBuilder<T>::Function& comp) {}

template <typename T>
UInt32<T> Vector<T>::Size() {
  return SizePtr(data).Load();
}

template <typename T>
Struct<T> Vector<T>::Get(const proxy::UInt32<T>& idx) {
  return Struct<T>(program_, content_,
                   program_.GetElementPtr(*struct_type, *data,
                                          {program_.ConstI32(0),
                                           program_.ConstI32(2), idx.Get()}));
}

template <typename T>
typename ProgramBuilder<T>::Value& Vector<T>::Create(
    const proxy::UInt32<T>& new_capacity) {
  auto& size_ptr = program_.GetElementPtr(
      *struct_type, program_.NullPtr(*struct_ptr_type),
      {program_.ConstI32(0), program_.ConstI32(2), new_capacity.Get()});
  auto& size = program_.PointerCast(size_ptr, program_.UI32Type());
  auto& ptr = program_.Malloc(size);
  return program_.PointerCast(ptr, *struct_ptr_type);
}

template <typename T>
void Vector<T>::Copy(typename ProgramBuilder<T>::Value* source,
                     typename ProgramBuilder<T>::Value* dest,
                     const proxy::UInt32<T>& size) {
  auto& old_data_ptr = program_.GetElementPtr(
      *struct_type, *source, {program_.ConstI32(0), program_.ConstI32(2)});
  auto& length_ptr = program_.GetElementPtr(
      *struct_type, program_.NullPtr(*struct_ptr_type),
      {program_.ConstI32(0), program_.ConstI32(2), size.Get()});
  auto& length = program_.PointerCast(length_ptr, program_.UI32Type());
  auto& new_data_ptr = program_.GetElementPtr(
      *struct_type, *dest, {program_.ConstI32(0), program_.ConstI32(2)});
  program_.Memcpy(program_.PointerCast(new_data_ptr, program_.I8Type()),
                  program_.PointerCast(old_data_ptr, program_.I8Type()),
                  length);
}

template <typename T>
Ptr<T, UInt32<T>> Vector<T>::CapacityPtr(
    typename ProgramBuilder<T>::Value* target) {
  return Ptr<T, UInt32<T>>(
      program_,
      program_.GetElementPtr(*struct_type, *target,
                             {program_.ConstI32(0), program_.ConstI32(0)}));
}

template <typename T>
Ptr<T, UInt32<T>> Vector<T>::SizePtr(
    typename ProgramBuilder<T>::Value* target) {
  return Ptr<T, UInt32<T>>(
      program_,
      program_.GetElementPtr(*struct_type, *target,
                             {program_.ConstI32(0), program_.ConstI32(1)}));
}

INSTANTIATE_ON_IR(Vector);

}  // namespace kush::compile::proxy