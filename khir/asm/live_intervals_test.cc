#include "khir/asm/live_intervals.h"

#include "gtest/gtest.h"

#include "khir/program_printer.h"

using namespace kush;
using namespace kush::khir;

TEST(LiveIntervalsTest, SingleBasicBlock) {
  ProgramBuilder program;
  auto type = program.I32Type();
  auto func = program.CreatePublicFunction(type, {type, type}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.AddI32(args[0], args[1]));

  auto res =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());

  khir::ProgramPrinter printer;
  program.Translate(printer);

  for (auto& x : res) {
    if (x.IsPrecolored()) {
      std::cerr << "Precolored: " << x.PrecoloredRegister() << std::endl;
    } else {
      std::cerr << "Value: " << x.Value().GetIdx() << std::endl;
    }
    std::cerr << ' ' << x.Start() << ' ' << x.End() << std::endl;
  }
}
