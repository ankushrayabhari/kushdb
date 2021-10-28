#include "khir/asm/linear_scan_reg_alloc.h"

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/forward_declare.h"
#include "compile/query_translator.h"
#include "compile/translators/translator_factory.h"
#include "end_to_end_test/schema.h"
#include "khir/asm/live_intervals.h"
#include "khir/program_builder.h"
#include "khir/program_printer.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/hash_join_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/order_by_operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "util/builder.h"

using namespace kush;
using namespace kush::khir;
using namespace kush::compile;
using namespace kush::plan;
using namespace kush::util;

TEST(LiveIntervalsTest, StoreInstructionForcedIntoRegister) {
  for (auto type_func : {&ProgramBuilder::I8Type, &ProgramBuilder::I16Type,
                         &ProgramBuilder::I32Type, &ProgramBuilder::I64Type,
                         &ProgramBuilder::F64Type}) {
    ProgramBuilder program;
    auto type = std::invoke(type_func, program);
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(type), type}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(args[0], args[1]);
    program.Return();

    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager());
    EXPECT_EQ(result.size(), 4);
    EXPECT_TRUE(result[2].IsRegister());
  }
}

TEST(LiveIntervalsTest, I8FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI8(cmp_type, args[0], program.ConstI8(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager());
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, I16FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI16(cmp_type, args[0], program.ConstI16(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager());
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, I32FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI32(cmp_type, args[0], program.ConstI32(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager());
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, I64FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI64(cmp_type, args[0], program.ConstI64(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager());
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, F64FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpF64(cmp_type, args[0], program.ConstF64(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager());
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 101);
  }
}

// TODO: reenable test when the pipeline refactor is complete
/*
TEST(LiveIntervalsTest, NoOverlappingLiveIntervals) {
  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> s1;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"id"});
      s1 = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    std::unique_ptr<Operator> s2;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"id"});
      s2 = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
    conditions.push_back(Eq(ColRef(s1, "id", 0), ColRef(s2, "id", 1)));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*s1, 0);
    schema.AddPassthroughColumns(*s2, 1);
    query =
        std::make_unique<OutputOperator>(std::make_unique<SkinnerJoinOperator>(
            std::move(schema), util::MakeVector(std::move(s1), std::move(s2)),
            std::move(conditions)));
  }

  khir::ProgramBuilder program;
  execution::PipelineBuilder pipeline_builder;

  ForwardDeclare(program);

  // Generate code for operator
  TranslatorFactory factory(program, pipeline_builder);
  auto translator = factory.Compute(op_);
  translator->Produce();

  auto assignments = LinearScanRegisterAlloc(program.GetFunction(func),
                                             program.GetTypeManager());

  auto live_intervals =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
  for (int i = 0; i < live_intervals.size(); i++) {
    const auto& x = live_intervals[i];
    for (int j = i + 1; j < live_intervals.size(); j++) {
      const auto& y = live_intervals[j];

      if (!x.IsPrecolored() && !y.IsPrecolored()) {
        if (!(x.End() < y.Start() || y.End() < x.Start())) {
          if (assignments[x.Value().GetIdx()].IsRegister() &&
              assignments[y.Value().GetIdx()].IsRegister()) {
            EXPECT_NE(assignments[x.Value().GetIdx()].Register(),
                      assignments[y.Value().GetIdx()].Register());
          }
        }
      }
    }
  }
}
*/