#include "compile/translators/cross_product_translator.h"

#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/struct.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/cross_product_operator.h"

namespace kush::compile {

CrossProductTranslator::CrossProductTranslator(
    const plan::CrossProductOperator& cross_product,
    khir::ProgramBuilder& program, execution::PipelineBuilder& pipeline_builder,
    execution::QueryState& state,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(cross_product, std::move(children)),
      cross_product_(cross_product),
      program_(program),
      pipeline_builder_(pipeline_builder),
      state_(state),
      expr_translator_(program, *this) {
  if (this->Children().size() != 2) {
    throw std::runtime_error("INVALID");
  }
}

void CrossProductTranslator::Produce(proxy::Pipeline& output) {
  // buffer the input
  proxy::StructBuilder packed(program_);
  const auto& child_schema =
      cross_product_.Children()[0].get().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type(), col.Expr().Nullable());
  }
  packed.Build();
  buffer_ = std::make_unique<proxy::Vector>(program_, state_, packed);

  // populate buffer
  proxy::Pipeline input(program_, pipeline_builder_);
  input.Init([&]() { buffer_->Init(); });
  input.Reset([&]() { buffer_->Reset(); });
  LeftChild().Produce(input);
  input.Build();

  // output
  output.Get().AddPredecessor(input.Get());
  RightChild().Produce(output);
}

void CrossProductTranslator::Consume(OperatorTranslator& src) {
  if (&src == &LeftChild()) {
    buffer_->PushBack().Pack(LeftChild().SchemaValues().Values());
    return;
  }

  auto size = buffer_->Size();
  proxy::Loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < size;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        this->LeftChild().SchemaValues().SetValues((*buffer_)[i].Unpack());

        this->values_.ResetValues();
        for (const auto& column : cross_product_.Schema().Columns()) {
          this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return loop.Continue(i + 1);
      });
}

}  // namespace kush::compile