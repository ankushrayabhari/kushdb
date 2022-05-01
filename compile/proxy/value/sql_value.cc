#include "compile/proxy/value/sql_value.h"

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "plan/expression/arithmetic_expression.h"

namespace kush::compile::proxy {

SQLValue::SQLValue(const Bool& value, const Bool& null)
    : program_(value.ProgramBuilder()),
      value_(value.ToPointer()),
      null_(null),
      type_(catalog::Type::Boolean()) {}

SQLValue::SQLValue(const Int16& value, const Bool& null)
    : program_(value.ProgramBuilder()),
      value_(value.ToPointer()),
      null_(null),
      type_(catalog::Type::SmallInt()) {}

SQLValue::SQLValue(const Int32& value, const Bool& null)
    : program_(value.ProgramBuilder()),
      value_(value.ToPointer()),
      null_(null),
      type_(catalog::Type::Int()) {}

SQLValue::SQLValue(const Int64& value, const Bool& null)
    : program_(value.ProgramBuilder()),
      value_(value.ToPointer()),
      null_(null),
      type_(catalog::Type::BigInt()) {}

SQLValue::SQLValue(const Date& value, const Bool& null)
    : program_(value.ProgramBuilder()),
      value_(value.ToPointer()),
      null_(null),
      type_(catalog::Type::Date()) {}

SQLValue::SQLValue(const Float64& value, const Bool& null)
    : program_(value.ProgramBuilder()),
      value_(value.ToPointer()),
      null_(null),
      type_(catalog::Type::Real()) {}

SQLValue::SQLValue(const String& value, const Bool& null)
    : program_(value.ProgramBuilder()),
      value_(value.ToPointer()),
      null_(null),
      type_(catalog::Type::Text()) {}

SQLValue::SQLValue(std::unique_ptr<IRValue> value, const catalog::Type& type,
                   const Bool& null)
    : program_(value->ProgramBuilder()),
      value_(std::move(value)),
      null_(null),
      type_(type) {}

std::unique_ptr<IRValue> CopyIRValue(const catalog::Type& t, IRValue& v) {
  switch (t.type_id) {
    case catalog::TypeId::BOOLEAN:
      return dynamic_cast<Bool&>(v).ToPointer();
    case catalog::TypeId::SMALLINT:
      return dynamic_cast<Int16&>(v).ToPointer();
    case catalog::TypeId::INT:
      return dynamic_cast<Int32&>(v).ToPointer();
    case catalog::TypeId::BIGINT:
      return dynamic_cast<Int64&>(v).ToPointer();
    case catalog::TypeId::DATE:
      return dynamic_cast<Date&>(v).ToPointer();
    case catalog::TypeId::REAL:
      return dynamic_cast<Float64&>(v).ToPointer();
    case catalog::TypeId::TEXT:
      return dynamic_cast<String&>(v).ToPointer();
  }

  throw std::runtime_error("Unknown type");
}

SQLValue::SQLValue(const SQLValue& rhs)
    : program_(rhs.program_),
      value_(CopyIRValue(rhs.type_, *rhs.value_)),
      null_(rhs.null_),
      type_(rhs.type_) {}

SQLValue::SQLValue(SQLValue&& rhs)
    : program_(rhs.program_),
      value_(std::move(rhs.value_)),
      null_(std::move(rhs.null_)),
      type_(rhs.type_) {}

SQLValue& SQLValue::operator=(const SQLValue& rhs) {
  assert(&program_ == &rhs.program_);
  value_ = CopyIRValue(rhs.type_, *rhs.value_);
  null_ = rhs.null_;
  type_ = rhs.type_;
  return *this;
}

SQLValue& SQLValue::operator=(SQLValue&& rhs) {
  assert(&program_ == &rhs.program_);
  value_ = std::move(rhs.value_);
  null_ = std::move(rhs.null_);
  type_ = rhs.type_;
  return *this;
}

Bool SQLValue::IsNull() const { return null_; }

IRValue& SQLValue::Get() const { return *value_; }

const catalog::Type& SQLValue::Type() const { return type_; }

khir::ProgramBuilder& SQLValue::ProgramBuilder() const { return program_; }

SQLValue SQLValue::GetNotNullable() const {
  return SQLValue(CopyIRValue(type_, *value_), type_,
                  proxy::Bool(program_, false));
}

}  // namespace kush::compile::proxy