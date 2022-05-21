#include "compile/translators/simd_scan_select_translator.h"

#include <memory>

#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/proxy/pipeline.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/expression/literal_expression.h"
#include "plan/operator/simd_scan_select_operator.h"

namespace kush::compile {

SimdScanSelectTranslator::SimdScanSelectTranslator(
    const plan::SimdScanSelectOperator& scan_select,
    khir::ProgramBuilder& program, execution::PipelineBuilder& pipeline_builder,
    execution::QueryState& state)
    : OperatorTranslator(scan_select, {}),
      scan_select_(scan_select),
      program_(program),
      pipeline_builder_(pipeline_builder),
      state_(state),
      expr_translator_(program, *this) {}

std::unique_ptr<proxy::DiskMaterializedBuffer>
SimdScanSelectTranslator::GenerateBuffer() {
  const auto& table = scan_select_.Relation();
  const auto& cols = scan_select_.ScanSchema().Columns();
  auto num_cols = cols.size();

  std::vector<std::unique_ptr<proxy::Iterable>> column_data;
  std::vector<std::unique_ptr<proxy::Iterable>> null_data;
  column_data.reserve(num_cols);
  null_data.reserve(num_cols);
  for (const auto& column : cols) {
    using catalog::TypeId;
    auto type = column.Expr().Type();
    auto path = table[column.Name()].Path();
    switch (type.type_id) {
      case TypeId::SMALLINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::SMALLINT>>(
                program_, state_, path, type));
        break;
      case TypeId::INT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::INT>>(
            program_, state_, path, type));
        break;
      case TypeId::ENUM:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::ENUM>>(
            program_, state_, path, type));
        break;
      case TypeId::BIGINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BIGINT>>(
                program_, state_, path, type));
        break;
      case TypeId::REAL:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::REAL>>(
            program_, state_, path, type));
        break;
      case TypeId::DATE:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::DATE>>(
            program_, state_, path, type));
        break;
      case TypeId::TEXT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::TEXT>>(
            program_, state_, path, type));
        break;
      case TypeId::BOOLEAN:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(
                program_, state_, path, type));
        break;
    }

    if (table[column.Name()].Nullable()) {
      null_data.push_back(std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(
          program_, state_, table[column.Name()].NullPath(),
          catalog::Type::Boolean()));
    } else {
      null_data.push_back(nullptr);
    }
  }

  return std::make_unique<proxy::DiskMaterializedBuffer>(
      program_, std::move(column_data), std::move(null_data));
}

void SimdScanSelectTranslator::Produce(proxy::Pipeline& output) {
  auto materialized_buffer = GenerateBuffer();

  const auto& table = scan_select_.Relation();
  const auto& cols = scan_select_.ScanSchema().Columns();
  const auto& filters = scan_select_.Filters();

  std::vector<std::unique_ptr<proxy::Iterable>> column_data(cols.size());
  for (int i = 0; i < cols.size(); i++) {
    if (filters[i].empty()) continue;
    const auto& column = cols[i];
    using catalog::TypeId;
    auto type = column.Expr().Type();
    auto path = table[column.Name()].Path();
    switch (type.type_id) {
      case TypeId::INT:
        column_data[i] = std::make_unique<proxy::ColumnData<TypeId::INT>>(
            program_, state_, path, type);
        break;
      case TypeId::ENUM:
        column_data[i] = std::make_unique<proxy::ColumnData<TypeId::ENUM>>(
            program_, state_, path, type);
        break;
      case TypeId::DATE:
        column_data[i] = std::make_unique<proxy::ColumnData<TypeId::DATE>>(
            program_, state_, path, type);
        break;
      default:
        throw std::runtime_error("Invalid column type for SIMD Scan");
    }

    if (table[column.Name()].Nullable() && type.type_id != TypeId::ENUM) {
      throw std::runtime_error("Invalid null column type for SIMD Scan");
    }
  }

  // Create a dummy pipeline for the input
  proxy::Pipeline input(program_, pipeline_builder_);
  input.Init([&]() {
    materialized_buffer->Init();
    for (int i = 0; i < cols.size(); i++) {
      if (filters[i].empty()) continue;
      column_data[i]->Init();
    }
  });
  input.Reset([&]() {
    materialized_buffer->Reset();
    for (int i = 0; i < cols.size(); i++) {
      if (filters[i].empty()) continue;
      column_data[i]->Reset();
    }
  });
  input.Size([&]() { return materialized_buffer->Size(); });
  input.Build();

  output.Body(input, [&](proxy::Int32 start, proxy::Int32 end) {
    proxy::Loop loop(
        program_, [&](auto& loop) { loop.AddLoopVariable(start); },
        [&](auto& loop) {
          auto i = loop.template GetLoopVariable<proxy::Int32>(0);
          return i <= end;
        },
        [&](auto& loop) {
          auto i = loop.template GetLoopVariable<proxy::Int32>(0);

          const int BUFFER_SIZE = 64;

          // declare 64 capacity buffer containing the valid tuple idx
          auto buffer_type =
              program_.ArrayType(program_.I32Type(), BUFFER_SIZE);
          auto buffer = program_.StaticGEP(
              buffer_type,
              program_.Global(
                  buffer_type,
                  program_.ConstantArray(
                      buffer_type, std::vector<khir::Value>(
                                       BUFFER_SIZE, program_.ConstI32(0)))),
              {0, 0});

          // if we are within 8 of the ending, just manually loop.
          // otherwise use SIMD
          auto tuple_idx_buffer_size = proxy::Ternary(
              program_, i >= end - 7,
              [&]() {
                proxy::Loop manual_loop(
                    program_,
                    [&](auto& manual_loop) {
                      manual_loop.AddLoopVariable(i);
                      manual_loop.AddLoopVariable(proxy::Int32(program_, 0));
                    },
                    [&](auto& manual_loop) {
                      auto tuple_idx =
                          manual_loop.template GetLoopVariable<proxy::Int32>(0);
                      auto buffer_size =
                          manual_loop.template GetLoopVariable<proxy::Int32>(1);
                      return tuple_idx <= end;
                    },
                    [&](auto& manual_loop) {
                      auto tuple_idx =
                          manual_loop.template GetLoopVariable<proxy::Int32>(0);
                      auto buffer_size =
                          manual_loop.template GetLoopVariable<proxy::Int32>(1);

                      for (int col_idx = 0; col_idx < column_data.size();
                           col_idx++) {
                        if (filters[col_idx].empty()) continue;

                        // load col_idx at tuple_idx
                        khir::Value data =
                            column_data[col_idx]->operator[](tuple_idx)->Get();

                        for (const auto& filter : filters[col_idx]) {
                          // rhs is guaranteed to be a constant
                          auto literal = dynamic_cast<plan::LiteralExpression*>(
                              &filter->RightChild());

                          int32_t value;
                          literal->Visit(
                              [](int16_t, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [&](int32_t v, bool null) {
                                if (null) {
                                  throw std::runtime_error(
                                      "Invalid literal for SIMD Select");
                                }
                                value = v;
                              },
                              [](int64_t, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [](double, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [](std::string, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [](bool, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [&](runtime::Date::DateBuilder db, bool null) {
                                if (null) {
                                  throw std::runtime_error(
                                      "Invalid literal for SIMD Select");
                                }
                                value = db.Build();
                              },
                              [&](int32_t v, int32_t enum_id, bool null) {
                                if (null) {
                                  throw std::runtime_error(
                                      "Invalid literal for SIMD Select");
                                }
                                value = v;
                              });

                          khir::Value filter_mask;
                          switch (filter.get()->OpType()) {
                            case plan::BinaryArithmeticExpressionType::EQ:
                              filter_mask =
                                  program_.CmpI32(khir::CompType::EQ, data,
                                                  program_.ConstI32(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::NEQ:
                              filter_mask =
                                  program_.CmpI32(khir::CompType::NE, data,
                                                  program_.ConstI32(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::LT:
                              filter_mask =
                                  program_.CmpI32(khir::CompType::LT, data,
                                                  program_.ConstI32(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::LEQ:
                              filter_mask =
                                  program_.CmpI32(khir::CompType::LE, data,
                                                  program_.ConstI32(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::GT:
                              filter_mask =
                                  program_.CmpI32(khir::CompType::GT, data,
                                                  program_.ConstI32(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::GEQ:
                              filter_mask =
                                  program_.CmpI32(khir::CompType::GE, data,
                                                  program_.ConstI32(value));
                              break;

                            default:
                              throw std::runtime_error(
                                  "Invalid filter type for SIMD Scan/Select");
                          }

                          proxy::Bool cond(program_, filter_mask);
                          proxy::If(program_, NOT, cond, [&]() {
                            manual_loop.Continue(tuple_idx + 1, buffer_size);
                          });
                        }
                      }

                      auto ptr = program_.DynamicGEP(program_.I32Type(), buffer,
                                                     buffer_size.Get(), {});
                      program_.StoreI32(ptr, tuple_idx.Get());

                      return manual_loop.Continue(tuple_idx + 1,
                                                  buffer_size + 1);
                    });

                auto tuple_idx =
                    manual_loop.template GetLoopVariable<proxy::Int32>(0);
                auto buffer_size =
                    manual_loop.template GetLoopVariable<proxy::Int32>(1);
                return std::array<proxy::Int32, 2>{tuple_idx, buffer_size};
              },
              [&]() {
                proxy::Loop simd_loop(
                    program_,
                    [&](auto& simd_loop) {
                      simd_loop.AddLoopVariable(i);
                      simd_loop.AddLoopVariable(proxy::Int32(program_, 0));
                    },
                    [&](auto& simd_loop) {
                      auto tuple_idx =
                          simd_loop.template GetLoopVariable<proxy::Int32>(0);
                      auto buffer_size =
                          simd_loop.template GetLoopVariable<proxy::Int32>(1);
                      // keep adding while 8 more entries exist with the
                      // base_table
                      auto cond2 = tuple_idx <= end - 7;
                      // AND we have space to add 8 more entries
                      auto cond1 = buffer_size <= BUFFER_SIZE - 8;
                      return cond1 && cond2;
                    },
                    [&](auto& simd_loop) {
                      auto tuple_idx =
                          simd_loop.template GetLoopVariable<proxy::Int32>(0);
                      auto buffer_size =
                          simd_loop.template GetLoopVariable<proxy::Int32>(1);
                      std::optional<khir::Value> mask;

                      for (int col_idx = 0; col_idx < column_data.size();
                           col_idx++) {
                        if (filters[col_idx].empty()) continue;

                        // simd load col_idx at tuple_idx
                        khir::Value data =
                            column_data[col_idx]->SimdLoad(tuple_idx);

                        for (const auto& filter : filters[col_idx]) {
                          // rhs is guaranteed to be a constant
                          auto literal = dynamic_cast<plan::LiteralExpression*>(
                              &filter->RightChild());

                          int32_t value;
                          literal->Visit(
                              [](int16_t, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [&](int32_t v, bool null) {
                                if (null) {
                                  throw std::runtime_error(
                                      "Invalid literal for SIMD Select");
                                }
                                value = v;
                              },
                              [](int64_t, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [](double, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [](std::string, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [](bool, bool) {
                                throw std::runtime_error(
                                    "Invalid literal for SIMD Select");
                              },
                              [&](runtime::Date::DateBuilder db, bool null) {
                                if (null) {
                                  throw std::runtime_error(
                                      "Invalid literal for SIMD Select");
                                }
                                value = db.Build();
                              },
                              [&](int32_t v, int32_t enum_id, bool null) {
                                if (null) {
                                  throw std::runtime_error(
                                      "Invalid literal for SIMD Select");
                                }
                                value = v;
                              });

                          khir::Value filter_mask;
                          switch (filter.get()->OpType()) {
                            case plan::BinaryArithmeticExpressionType::EQ:
                              filter_mask = program_.CmpI32Vec8(
                                  khir::CompType::EQ, data,
                                  program_.ConstI32Vec8(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::NEQ:
                              filter_mask = program_.CmpI32Vec8(
                                  khir::CompType::NE, data,
                                  program_.ConstI32Vec8(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::LT:
                              filter_mask = program_.CmpI32Vec8(
                                  khir::CompType::LT, data,
                                  program_.ConstI32Vec8(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::LEQ:
                              filter_mask = program_.CmpI32Vec8(
                                  khir::CompType::LE, data,
                                  program_.ConstI32Vec8(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::GT:
                              filter_mask = program_.CmpI32Vec8(
                                  khir::CompType::GT, data,
                                  program_.ConstI32Vec8(value));
                              break;

                            case plan::BinaryArithmeticExpressionType::GEQ:
                              filter_mask = program_.CmpI32Vec8(
                                  khir::CompType::GE, data,
                                  program_.ConstI32Vec8(value));
                              break;

                            default:
                              throw std::runtime_error(
                                  "Invalid filter type for SIMD Scan/Select");
                          }

                          if (mask.has_value()) {
                            if (scan_select_.Conjunction()) {
                              mask =
                                  program_.AndI1Vec8(mask.value(), filter_mask);
                            } else {
                              mask =
                                  program_.OrI1Vec8(mask.value(), filter_mask);
                            }
                          } else {
                            mask = filter_mask;
                          }
                        }
                      }

                      auto extracted_mask =
                          program_.ExtractMaskI1Vec8(mask.value());
                      auto popcount = program_.PopcountI64(extracted_mask);
                      auto permute_idx = program_.LoadI32Vec8(
                          program_.MaskToPermutePtr(extracted_mask));

                      auto base_idx = program_.I32Vec8ConvI32(tuple_idx.Get());
                      auto offsets =
                          program_.ConstI32Vec8({0, 1, 2, 3, 4, 5, 6, 7});
                      auto idx = program_.AddI32Vec8(base_idx, offsets);

                      auto permuted = program_.PermuteI32Vec8(idx, permute_idx);
                      auto ptr = program_.PointerCast(
                          program_.DynamicGEP(program_.I32Type(), buffer,
                                              buffer_size.Get(), {}),
                          program_.PointerType(program_.I32Vec8Type()));
                      program_.MaskStoreI32Vec8(ptr, permuted, popcount);

                      return simd_loop.Continue(
                          // processed 8 tuples
                          tuple_idx + 8,
                          // only added popcount to the buffer
                          buffer_size +
                              proxy::Int32(program_,
                                           program_.I32TruncI64(popcount)));
                    });

                auto tuple_idx =
                    simd_loop.template GetLoopVariable<proxy::Int32>(0);
                auto buffer_size =
                    simd_loop.template GetLoopVariable<proxy::Int32>(1);
                return std::array<proxy::Int32, 2>{tuple_idx, buffer_size};
              });

          // output tuples
          auto tuple_idx = tuple_idx_buffer_size[0];
          auto buffer_size = tuple_idx_buffer_size[1];

          proxy::Loop output_loop(
              program_,
              [&](auto& output_loop) {
                output_loop.AddLoopVariable(proxy::Int32(program_, 0));
              },
              [&](auto& output_loop) {
                auto buffer_idx =
                    output_loop.template GetLoopVariable<proxy::Int32>(0);
                return buffer_idx < buffer_size;
              },
              [&](auto& output_loop) {
                auto buffer_idx =
                    output_loop.template GetLoopVariable<proxy::Int32>(0);

                auto ptr = program_.DynamicGEP(program_.I32Type(), buffer,
                                               buffer_idx.Get(), {});
                proxy::Int32 tuple_idx(program_, program_.LoadI32(ptr));
                this->virtual_values_.SetValues(
                    (*materialized_buffer)[tuple_idx]);

                this->values_.ResetValues();
                for (const auto& column : scan_select_.Schema().Columns()) {
                  this->values_.AddVariable(
                      expr_translator_.Compute(column.Expr()));
                }

                if (auto parent = this->Parent()) {
                  parent->get().Consume(*this);
                }

                return output_loop.Continue(buffer_idx + 1);
              });

          return loop.Continue(tuple_idx);
        });
  });
}

void SimdScanSelectTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile