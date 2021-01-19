#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename Impl>
class Bool : public Value<Impl> {
 public:
  Bool(ProgramBuilder<Impl>& program,
       typename ProgramBuilder<Impl>::Value& value);
  Bool(ProgramBuilder<Impl>& program, bool value);

  typename ProgramBuilder<Impl>::Value& Get() const override;
  Bool operator!();
  Bool operator==(const Bool& rhs);
  Bool operator!=(const Bool& rhs);

 private:
  ProgramBuilder<Impl>& program_;
  typename ProgramBuilder<Impl>::Value& value_;
};

}  // namespace kush::compile::proxy