#include "compile/translators/permutable_skinner_scan_select_translator.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/disk_column_index.h"
#include "compile/proxy/skinner_scan_select_executor.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/predicate_column_collector.h"
#include "khir/program_builder.h"
#include "plan/operator/skinner_scan_select_operator.h"

namespace kush::compile {

class PredicateFunction {
 public:
  PredicateFunction(khir::ProgramBuilder& program,
                    std::function<void(PredicateFunction& p)> body)
      : program_(program) {
    auto current_block = program_.CurrentBlock();
    func_ = program_.CreatePublicFunction(
        program_.I1Type(), {},
        "permute_predicate_" + std::to_string(predicate_++));
    body(*this);
    program_.SetCurrentBlock(current_block);
  }

  void Return(const proxy::Bool& b) { program_.Return(b.Get()); }

  khir::FunctionRef Get() { return func_.value(); }

 private:
  khir::ProgramBuilder& program_;
  std::optional<khir::FunctionRef> func_;
  static int predicate_;
};

int PredicateFunction::predicate_ = 0;

class BaseFunction {
 public:
  BaseFunction(khir::ProgramBuilder& program,
               std::function<void(proxy::Int32, proxy::Int32)> body)
      : program_(program) {
    auto current_block = program_.CurrentBlock();
    func_ = program_.CreatePublicFunction(
        program_.I32Type(), {program_.I32Type(), program_.I32Type()},
        "base_" + std::to_string(funcid_++));

    auto args = program_.GetFunctionArguments(func_.value());
    proxy::Int32 budget(program_, args[0]);
    proxy::Int32 next_tuple(program_, args[1]);

    body(budget, next_tuple);
    program_.SetCurrentBlock(current_block);
  }

  khir::FunctionRef Get() { return func_.value(); }

 private:
  khir::ProgramBuilder& program_;
  std::optional<khir::FunctionRef> func_;
  static int funcid_;
};
int BaseFunction::funcid_ = 0;

PermutableSkinnerScanSelectTranslator::PermutableSkinnerScanSelectTranslator(
    const plan::SkinnerScanSelectOperator& scan_select,
    khir::ProgramBuilder& program)
    : OperatorTranslator(scan_select, {}),
      scan_select_(scan_select),
      program_(program),
      expr_translator_(program, *this) {}

std::unique_ptr<proxy::DiskMaterializedBuffer>
PermutableSkinnerScanSelectTranslator::GenerateBuffer() {
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
            std::make_unique<proxy::ColumnData<TypeId::SMALLINT>>(program_,
                                                                  path, type));
        break;
      case TypeId::INT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::INT>>(
            program_, path, type));
        break;
      case TypeId::ENUM:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::ENUM>>(
            program_, path, type));
        break;
      case TypeId::BIGINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BIGINT>>(program_, path,
                                                                type));
        break;
      case TypeId::REAL:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::REAL>>(
            program_, path, type));
        break;
      case TypeId::DATE:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::DATE>>(
            program_, path, type));
        break;
      case TypeId::TEXT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::TEXT>>(
            program_, path, type));
        break;
      case TypeId::BOOLEAN:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(program_, path,
                                                                 type));
        break;
    }

    if (table[column.Name()].Nullable()) {
      null_data.push_back(std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(
          program_, table[column.Name()].NullPath(), catalog::Type::Boolean()));
    } else {
      null_data.push_back(nullptr);
    }
  }

  return std::make_unique<proxy::DiskMaterializedBuffer>(
      program_, std::move(column_data), std::move(null_data));
}

bool PermutableSkinnerScanSelectTranslator::HasIndex(int col_idx) const {
  const auto& table = scan_select_.Relation();
  const auto& cols = scan_select_.ScanSchema().Columns();
  auto col_name = cols[col_idx].Name();
  return table[col_name].HasIndex();
}

std::unique_ptr<proxy::ColumnIndex>
PermutableSkinnerScanSelectTranslator::GenerateIndex(
    khir::ProgramBuilder& program, int col_idx) {
  const auto& table = scan_select_.Relation();
  const auto& cols = scan_select_.ScanSchema().Columns();
  const auto& column = cols[col_idx];
  auto type = column.Expr().Type();
  auto path = table[column.Name()].IndexPath();
  using catalog::TypeId;
  switch (type.type_id) {
    case TypeId::SMALLINT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::SMALLINT>>(program,
                                                                        path);
    case TypeId::INT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::INT>>(program,
                                                                   path);
    case TypeId::ENUM:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::ENUM>>(program,
                                                                    path);
    case TypeId::BIGINT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::BIGINT>>(program,
                                                                      path);
    case TypeId::REAL:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::REAL>>(program,
                                                                    path);
    case TypeId::DATE:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::DATE>>(program,
                                                                    path);
    case TypeId::TEXT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::TEXT>>(program,
                                                                    path);
    case TypeId::BOOLEAN:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::BOOLEAN>>(program,
                                                                       path);
  }
}

void PermutableSkinnerScanSelectTranslator::Produce() {
  const auto& scan_schema_columns = scan_select_.ScanSchema().Columns();
  auto conditions = scan_select_.Filters();

  auto materialized_buffer = GenerateBuffer();
  materialized_buffer->Init();

  // 1. Setup predicate evaluation
  // Collect predicate columns.
  absl::flat_hash_set<int> predicate_columns;
  for (int i = 0; i < conditions.size(); i++) {
    const auto& condition = conditions[i].get();

    ScanSelectPredicateColumnCollector collector;
    condition.Accept(collector);
    auto cond = collector.PredicateColumns();

    for (const auto& col_ref : cond) {
      auto key = col_ref.get().GetColumnIdx();
      if (!predicate_columns.contains(key)) {
        predicate_columns.insert(key);
      }
    }
  }

  // Generate predicate struct.
  auto predicate_struct = std::make_unique<proxy::StructBuilder>(program_);
  absl::flat_hash_map<int, int> col_to_predicate_struct_field;
  int pred_struct_size = 0;
  for (auto column_idx : predicate_columns) {
    col_to_predicate_struct_field[column_idx] = pred_struct_size++;
    const auto& column_schema = scan_schema_columns[column_idx];
    predicate_struct->Add(column_schema.Expr().Type(),
                          column_schema.Expr().Nullable());
  }
  predicate_struct->Build();

  proxy::Struct global_predicate_struct(
      program_, *predicate_struct,
      program_.Global(
          predicate_struct->Type(),
          program_.ConstantStruct(predicate_struct->Type(),
                                  predicate_struct->DefaultValues())));

  this->virtual_values_.ResetValues();
  for (int i = 0; i < scan_schema_columns.size(); i++) {
    const auto& type = scan_schema_columns[i].Expr().Type();
    switch (type.type_id) {
      case catalog::TypeId::SMALLINT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Int16(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::TypeId::INT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Int32(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::TypeId::DATE:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Date(program_, runtime::Date::DateBuilder(2000, 1, 1)),
            proxy::Bool(program_, false)));
        break;
      case catalog::TypeId::BIGINT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Int64(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::TypeId::BOOLEAN:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Bool(program_, false), proxy::Bool(program_, false)));
        break;
      case catalog::TypeId::REAL:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Float64(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::TypeId::TEXT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::String::Global(program_, ""), proxy::Bool(program_, false)));
        break;
      case catalog::TypeId::ENUM:
        this->virtual_values_.AddVariable(
            proxy::SQLValue(proxy::Enum(program_, type.enum_id, -1),
                            proxy::Bool(program_, false)));
        break;
    }
  }

  // Generate function that will evaluate each predicate.
  std::vector<PredicateFunction> predicate_fns;
  for (int i = 0; i < conditions.size(); i++) {
    predicate_fns.emplace_back(program_, [&](auto& func) {
      // Unpack the predicate struct.
      auto column_values = global_predicate_struct.Unpack();

      for (const auto& [col_idx, field] : col_to_predicate_struct_field) {
        // Set the ColumnIdx value of the ChildIdx operator to be the
        // unpacked value.
        this->virtual_values_.SetValue(col_idx, column_values[field]);
      }

      auto value = expr_translator_.Compute(conditions[i].get());

      proxy::If(program_, value.IsNull(),
                [&]() { func.Return(proxy::Bool(program_, false)); });

      func.Return(static_cast<proxy::Bool&>(value.Get()));
    });
  }

  // Find index evaluatable predicates
  // In this case, equality predicates with a columnref and literal.
  absl::flat_hash_map<int, std::unique_ptr<proxy::ColumnIndex>>
      index_predicate_to_column_index;
  absl::flat_hash_map<int, const plan::LiteralExpression*>
      index_predicate_to_literal_values;
  for (int i = 0; i < conditions.size(); i++) {
    if (auto eq = dynamic_cast<const plan::BinaryArithmeticExpression*>(
            &conditions[i].get())) {
      if (eq->OpType() == plan::BinaryArithmeticExpressionType::EQ) {
        auto left = &eq->LeftChild();
        auto right = &eq->RightChild();

        if (auto r =
                dynamic_cast<const plan::VirtualColumnRefExpression*>(right)) {
          if (auto l = dynamic_cast<const plan::LiteralExpression*>(left)) {
            std::swap(left, right);
          }
        }

        if (auto l =
                dynamic_cast<const plan::VirtualColumnRefExpression*>(left)) {
          if (auto r = dynamic_cast<const plan::LiteralExpression*>(right)) {
            if (HasIndex(l->GetColumnIdx())) {
              index_evaluatable_predicates_.push_back(i);
              index_predicate_to_column_index[i] =
                  GenerateIndex(program_, l->GetColumnIdx());
              index_predicate_to_column_index[i]->Init();
              index_predicate_to_literal_values[i] = r;
            }
          }
        }
      }
    }
  }

  // Setup full list of index buckets
  proxy::ColumnIndexBucketArray index_bucket_array(
      program_, index_evaluatable_predicates_.size());
  for (int i : index_evaluatable_predicates_) {
    auto literal =
        expr_translator_.Compute(*index_predicate_to_literal_values[i]);
    auto bucket = index_predicate_to_column_index[i]->GetBucket(literal.Get());
    proxy::If(program_, bucket.DoesNotExist(), [&]() { program_.Return(); });
    index_bucket_array.PushBack(bucket);
  }

  // Setup valid index list.
  std::vector<khir::Value> initial_index_values(
      index_evaluatable_predicates_.size(), program_.ConstI32(0));
  auto index_array_type = program_.ArrayType(
      program_.I32Type(), index_evaluatable_predicates_.size());
  auto index_array = program_.Global(
      index_array_type,
      program_.ConstantArray(index_array_type, initial_index_values));

  // Setup idx array.
  auto index_array_size =
      program_.Global(program_.I32Type(), program_.ConstI32(0));

  // Setup function pointer array.
  auto predicate_ptr_ty =
      program_.PointerType(program_.FunctionType(program_.I1Type(), {}));
  std::vector<khir::Value> initial_predicate_array;
  for (auto& f : predicate_fns) {
    initial_predicate_array.push_back(program_.GetFunctionPointer(f.Get()));
  }
  auto predicate_array_type =
      program_.ArrayType(predicate_ptr_ty, predicate_fns.size());
  auto predicate_array = program_.Global(
      predicate_array_type,
      program_.ConstantArray(predicate_array_type, initial_predicate_array));

  // Setup idx array.
  auto progress_idx = program_.Global(program_.I32Type(), program_.ConstI32(0));

  // Main function
  BaseFunction main_func(program_, [&](proxy::Int32 initial_budget,
                                       proxy::Int32 next_tuple) {
    auto cardinality = materialized_buffer->Size();
    proxy::Int32 current_index_size(program_,
                                    program_.LoadI32(index_array_size));
    auto final_budget = proxy::Ternary(
        program_, current_index_size > 0,
        [&]() {
          index_bucket_array.InitSortedIntersection(next_tuple);

          int result_max_size = 64;
          auto result_array_type =
              program_.ArrayType(program_.I32Type(), result_max_size);
          std::vector<khir::Value> initial_result_values(result_max_size,
                                                         program_.ConstI32(0));
          auto result_array = program_.Global(
              result_array_type,
              program_.ConstantArray(result_array_type, initial_result_values));
          auto result =
              program_.StaticGEP(result_array_type, result_array, {0, 0});

          auto result_initial_size =
              index_bucket_array.PopulateSortedIntersectionResult(
                  result, result_max_size,
                  program_.StaticGEP(index_array_type, index_array, {0, 0}),
                  current_index_size);

          proxy::Loop loop(
              program_,
              [&](auto& loop) {
                loop.AddLoopVariable(initial_budget);
                loop.AddLoopVariable(result_initial_size);
              },
              [&](auto& loop) {
                auto result_size =
                    loop.template GetLoopVariable<proxy::Int32>(1);
                return result_size > 0;
              },
              [&](auto& loop) {
                auto budget = loop.template GetLoopVariable<proxy::Int32>(0);
                auto result_size =
                    loop.template GetLoopVariable<proxy::Int32>(1);

                // loop over all elements in result
                proxy::Loop result_loop(
                    program_,
                    [&](auto& result_loop) {
                      result_loop.AddLoopVariable(proxy::Int32(program_, 0));
                      result_loop.AddLoopVariable(budget);
                    },
                    [&](auto& result_loop) {
                      auto bucket_idx =
                          result_loop.template GetLoopVariable<proxy::Int32>(0);
                      return bucket_idx < result_size;
                    },
                    [&](auto& result_loop) {
                      auto bucket_idx =
                          result_loop.template GetLoopVariable<proxy::Int32>(0);
                      auto budget =
                          result_loop.template GetLoopVariable<proxy::Int32>(
                              1) -
                          1;
                      auto next_tuple = SortedIntersectionResultGet(
                          program_, result, bucket_idx);

                      proxy::If(program_, budget == 0, [&]() {
                        program_.StoreI32(progress_idx, next_tuple.Get());
                        program_.Return(program_.ConstI32(-2));
                      });

                      // Get next tuple
                      auto values = (*materialized_buffer)[next_tuple];

                      // Store each of this table's predicate column
                      // values into the global_predicate_struct.
                      for (auto [col_idx, field] :
                           col_to_predicate_struct_field) {
                        global_predicate_struct.Update(field, values[col_idx]);
                      }

                      // Check all predicates
                      auto num_predicates =
                          proxy::Int32(program_, predicate_fns.size()) -
                          current_index_size;
                      proxy::Loop inner_loop(
                          program_,
                          [&](auto& inner_loop) {
                            inner_loop.AddLoopVariable(
                                proxy::Int32(program_, 0));
                            inner_loop.AddLoopVariable(budget);
                          },
                          [&](auto& inner_loop) {
                            auto i =
                                inner_loop
                                    .template GetLoopVariable<proxy::Int32>(0);
                            return i < num_predicates;
                          },
                          [&](auto& inner_loop) {
                            auto i =
                                inner_loop
                                    .template GetLoopVariable<proxy::Int32>(0);
                            auto budget =
                                inner_loop
                                    .template GetLoopVariable<proxy::Int32>(1) -
                                1;

                            proxy::If(program_, budget == 0, [&]() {
                              program_.StoreI32(progress_idx, next_tuple.Get());
                              program_.Return(program_.ConstI32(-2));
                            });

                            auto predicate =
                                proxy::SkinnerScanSelectExecutor::GetFn(
                                    program_,
                                    program_.StaticGEP(predicate_array_type,
                                                       predicate_array, {0, 0}),
                                    i);

                            proxy::Bool value(program_,
                                              program_.Call(predicate, {}));
                            proxy::If(program_, !value, [&]() {
                              result_loop.Continue(bucket_idx + 1, budget);
                            });

                            return inner_loop.Continue(i + 1, budget);
                          });

                      this->virtual_values_.SetValues(values);
                      this->values_.ResetValues();
                      for (const auto& column :
                           scan_select_.Schema().Columns()) {
                        this->values_.AddVariable(
                            expr_translator_.Compute(column.Expr()));
                      }

                      if (auto parent = this->Parent()) {
                        parent->get().Consume(*this);
                      }

                      auto final_budget =
                          inner_loop.template GetLoopVariable<proxy::Int32>(1);
                      return result_loop.Continue(bucket_idx + 1, final_budget);
                    });

                auto next_budget =
                    result_loop.template GetLoopVariable<proxy::Int32>(1);
                auto next_result_size =
                    index_bucket_array.PopulateSortedIntersectionResult(
                        result, result_max_size,
                        program_.StaticGEP(index_array_type, index_array,
                                           {0, 0}),
                        current_index_size);
                return loop.Continue(next_budget, next_result_size);
              });

          auto final_budget = loop.template GetLoopVariable<proxy::Int32>(0);
          program_.StoreI32(progress_idx, cardinality.Get());
          return final_budget;
        },
        [&]() {
          proxy::Loop loop(
              program_,
              [&](auto& loop) {
                loop.AddLoopVariable(next_tuple);
                loop.AddLoopVariable(initial_budget);
              },
              [&](auto& loop) {
                auto i = loop.template GetLoopVariable<proxy::Int32>(0);
                return i < cardinality;
              },
              [&](auto& loop) {
                auto next_tuple =
                    loop.template GetLoopVariable<proxy::Int32>(0);
                auto budget = loop.template GetLoopVariable<proxy::Int32>(1);

                // Get ith value
                auto values = (*materialized_buffer)[next_tuple];

                // Store each of this table's predicate column
                // values into the global_predicate_struct.
                for (auto [col_idx, field] : col_to_predicate_struct_field) {
                  global_predicate_struct.Update(field, values[col_idx]);
                }

                // Check all predicates
                proxy::Loop inner_loop(
                    program_,
                    [&](auto& inner_loop) {
                      inner_loop.AddLoopVariable(proxy::Int32(program_, 0));
                      inner_loop.AddLoopVariable(budget);
                    },
                    [&](auto& inner_loop) {
                      auto i =
                          inner_loop.template GetLoopVariable<proxy::Int32>(0);
                      return i < predicate_fns.size();
                    },
                    [&](auto& inner_loop) {
                      auto i =
                          inner_loop.template GetLoopVariable<proxy::Int32>(0);
                      auto budget =
                          inner_loop.template GetLoopVariable<proxy::Int32>(1) -
                          1;

                      proxy::If(program_, budget == 0, [&]() {
                        program_.StoreI32(progress_idx, next_tuple.Get());
                        program_.Return(program_.ConstI32(-2));
                      });

                      auto predicate = proxy::SkinnerScanSelectExecutor::GetFn(
                          program_,
                          program_.StaticGEP(predicate_array_type,
                                             predicate_array, {0, 0}),
                          i);

                      proxy::Bool value(program_, program_.Call(predicate, {}));

                      proxy::If(program_, !value, [&]() {
                        loop.Continue(next_tuple + 1, budget);
                      });

                      return inner_loop.Continue(i + 1, budget);
                    });

                this->virtual_values_.SetValues(values);
                this->values_.ResetValues();
                for (const auto& column : scan_select_.Schema().Columns()) {
                  this->values_.AddVariable(
                      expr_translator_.Compute(column.Expr()));
                }

                if (auto parent = this->Parent()) {
                  parent->get().Consume(*this);
                }

                return loop.Continue(next_tuple + 1, budget);
              });

          auto final_budget = loop.template GetLoopVariable<proxy::Int32>(1);
          program_.StoreI32(progress_idx, cardinality.Get());
          return final_budget;
        });

    program_.Return(final_budget.Get());
  });

  auto main_func_ptr = program_.GetFunctionPointer(main_func.Get());

  auto cardinality = materialized_buffer->Size();
  program_.StoreI32(progress_idx, cardinality.Get());

  auto index_array_ptr =
      program_.StaticGEP(index_array_type, index_array, {0, 0});

  auto predicate_array_ptr =
      program_.StaticGEP(predicate_array_type, predicate_array, {0, 0});

  proxy::SkinnerScanSelectExecutor::ExecutePermutableScanSelect(
      program_, index_evaluatable_predicates_, main_func_ptr, index_array_ptr,
      index_array_size, predicate_array_ptr, predicate_fns.size(),
      progress_idx);

  materialized_buffer->Reset();
}

void PermutableSkinnerScanSelectTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile