#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
class Vector {
 public:
  Vector(ProgramBuilder<T>& program, StructBuilder<T>& content);
  ~Vector();

  Struct<T> Append();
  void Sort(typename ProgramBuilder<T>::Function& comp);
  UInt32<T> Size();
  Struct<T> Get(const proxy::UInt32<T>& idx);

 private:
  typename ProgramBuilder<T>::Value& Create(
      const proxy::UInt32<T>& new_capacity);

  void Copy(typename ProgramBuilder<T>::Value* source,
            typename ProgramBuilder<T>::Value* dest,
            const proxy::UInt32<T>& size);
  Ptr<T, UInt32<T>> CapacityPtr(typename ProgramBuilder<T>::Value* target);
  Ptr<T, UInt32<T>> SizePtr(typename ProgramBuilder<T>::Value* target);

  ProgramBuilder<T>& program_;
  StructBuilder<T>& content_;
  typename ProgramBuilder<T>::Type* struct_type;
  typename ProgramBuilder<T>::Type* struct_ptr_type;
  typename ProgramBuilder<T>::Type* content_type;
  typename ProgramBuilder<T>::Value* data;
};

}  // namespace kush::compile::proxy