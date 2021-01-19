#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename Impl>
class Boolean : public Value<Impl> {
 public:
  Boolean(ProgramBuilder<Impl>& program,
          typename ProgramBuilder<Impl>::Value& value);
  Boolean(ProgramBuilder<Impl>& program, bool value);

  typename ProgramBuilder<Impl>::Value& Get() const override;
  Boolean operator!();
  Boolean operator==(const Boolean& rhs);
  Boolean operator!=(const Boolean& rhs);

 private:
  ProgramBuilder<Impl>& program_;
  typename ProgramBuilder<Impl>::Value& value_;
};

}  // namespace kush::compile::proxy