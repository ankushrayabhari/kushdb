#include "compile/program_builder.h"
#include "compile/program_registry.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename Impl>
Bool<Impl>::Bool(ProgramBuilder<Impl>& program,
                 typename ProgramBuilder<Impl>::Value& value)
    : program_(program), value_(value) {}

template <typename Impl>
Bool<Impl>::Bool(ProgramBuilder<Impl>& program, bool value)
    : program_(program),
      value_(value ? program_.ConstI8(1) : program_.ConstI8(0)) {}

template <typename Impl>
typename ProgramBuilder<Impl>::Value& Bool<Impl>::Get() const {
  return value_;
}

template <typename Impl>
Bool<Impl> Bool<Impl>::operator!() {
  return Bool(program_, program_.LNotI8(value_));
}

template <typename Impl>
Bool<Impl> Bool<Impl>::operator==(const Bool<Impl>& rhs) {
  return Bool(program_,
              program_.CmpI8(Impl::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename Impl>
Bool<Impl> Bool<Impl>::operator!=(const Bool<Impl>& rhs) {
  return Bool(program_,
              program_.CmpI8(Impl::CompType::ICMP_NE, value_, rhs.value_));
}

INSTANTIATE_ON_BACKENDS(Bool);

}  // namespace kush::compile::proxy