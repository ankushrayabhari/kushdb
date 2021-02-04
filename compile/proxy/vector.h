#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

template <typename T>
class Vector {
 public:
  Vector(ProgramBuilder<T> program, ProgramBuilder<T>::Type& type)
      : program_(program) {
    struct_type = program.StructType({
        program.UI32Type(),      // capacity
        program.UI32Type(),      // size
        program.ArrayType(type)  // data
    });
    struct_ptr_type = program.PointerType(struct_type);

    // create the struct of initial capacity 2
    auto& data = Create(program.ConstUI32(2));

    // 2 -> capacity
    auto& capacity_ptr = program.GetElementPtr(struct_ptr_type, data, 0, 0);
    program.Store(capacity_ptr, program.ConstUI32(2));

    // 0 -> size
    auto& size_ptr = program.GetElementPtr(struct_ptr_type, data, 0, 1);
    program.Store(size_ptr, 0);
  }

  ~Vector() { program.Free(data); }

  // Returns the pointer to the pushed element
  typename ProgramBuilder<T>::Value& Append() {
    auto& capacity_ptr = program.GetElementPtr(struct_ptr_type, data, 0, 0);
    auto capacity = proxy::UInt32<T>(program_, program.Load(capacity_ptr));

    auto& size_ptr = program.GetElementPtr(struct_ptr_type, data, 0, 1);
    auto size = proxy::UInt32<T>(program_, program.Load(size_ptr));

    auto should_resize = capacity >= size;

    proxy::If<T> check(*should_resize, [&]() {
      auto new_capacity = capacity * proxy::UInt32<T>(program_, 2);
      auto& new_data = Create(new_capacity->Get());

      // 2 * old capacity -> capacity
      auto& capacity_ptr =
          program.GetElementPtr(struct_ptr_type, new_data, 0, 0);
      program.Store(capacity_ptr, new_capacity->Get());

      // old size -> size
      auto& size_ptr = program.GetElementPtr(struct_ptr_type, new_data, 0, 1);
      program.Store(size_ptr, size.Get());

      // memcopy
      auto& old_data_ptr = program.GetElementPtr(struct_ptr_type, data, 0, 2);
      auto& length_ptr = program.GetElementPtr(
          struct_ptr_type, program.NullPtr(),
          {program.ConstI32(0), program.ConstI32(2), capacity.Get()});
      auto& length = program.PointerCast(size, program.UI32Type());
      auto& new_data_ptr =
          program.GetElementPtr(struct_ptr_type, new_data, 0, 2);
      program.memcpy(program.PointerCast(new_data_ptr, program.I8Type()),
                     program.PointerCast(old_data_ptr, program.I8Type()),
                     length);
    });

    return check.Phi(data, new_data);
  }

  void Sort() {}

 private:
  typename ProgramBuilder<T>::Value& Create(
      typename ProgramBuilder<T>::Value& capacity) {
    auto& size_ptr = program.GetElementPtr(
        struct_ptr_type, program.NullPtr(),
        {program.ConstI32(0), program.ConstI32(2), capacity});
    auto& size = program.PointerCast(size, program.UI32Type());
    auto& ptr = program.Malloc(size);
    return program.PointerCast(ptr, struct_ptr_type);
  }

  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Type& struct_type;
  typename ProgramBuilder<T>::Type& struct_ptr_type;
};

}  // namespace kush::compile::proxy