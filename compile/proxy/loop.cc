#include "compile/proxy/loop.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
IndexLoop<T>::IndexLoop(ProgramBuilder<T>& program,
                        std::function<UInt32<T>()> init,
                        std::function<Bool<T>(UInt32<T>&)> cond,
                        std::function<UInt32<T>(UInt32<T>&)> body)
    : program_(program) {
  // Create preheader and get initial value
  auto& preheader = program.CurrentBlock();
  auto initial_value = init();

  // create the header
  auto& header_block = program.GenerateBlock();
  program.Branch(header_block);
  program.SetCurrentBlock(header_block);

  // loop variable
  auto& phi = program.Phi(program.I32Type());

  // initial value from preheader
  program.AddToPhi(phi, initial_value.Get(), preheader);
  auto loop_var = UInt32<T>(program, phi);

  // check the condition
  auto c = cond(loop_var);

  // jump to either body or end
  auto& body_block = program.GenerateBlock();
  auto& end_block = program.GenerateBlock();
  program.Branch(c.Get(), body_block, end_block);

  // run body and get next value for loop var
  // jump back to header
  program.SetCurrentBlock(body_block);
  auto updated_value = body(loop_var);
  program.AddToPhi(phi, updated_value.Get(), program.CurrentBlock());
  program.Branch(header_block);

  // set block to end
  program.SetCurrentBlock(end_block);
}

INSTANTIATE_ON_IR(IndexLoop);

}  // namespace kush::compile::proxy