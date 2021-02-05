#include "compile/proxy/loop.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
IndexLoop<T>::IndexLoop(ProgramBuilder<T>& program,
                        std::function<UInt32<T>()> init,
                        std::function<Bool<T>(UInt32<T>&)> cond,
                        std::function<UInt32<T>(UInt32<T>&)> body)
    : program_(program) {
  auto& init_block = program.CurrentBlock();

  // create the loop variable
  auto& phi = program.Phi();
  program.AddToPhi(phi, init().Get(), init_block);
  auto loop_var = UInt32<T>(program, phi);

  // check the condition
  If<T>(program, cond(loop_var), [&]() {
    auto& body_block = program.CurrentBlock();
    auto updated_value = body(loop_var);
    program.AddToPhi(phi, updated_value.Get(), body_block);
  });
}

INSTANTIATE_ON_IR(IndexLoop);

}  // namespace kush::compile::proxy