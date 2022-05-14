#include "khir/asm/live_intervals.h"

#include <iostream>
#include <stack>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "khir/asm/loops.h"
#include "khir/asm/register.h"
#include "khir/instruction.h"
#include "khir/program.h"
#include "util/union_find.h"

namespace std {
template <>
struct hash<kush::khir::Value> {
  size_t operator()(const kush::khir::Value& k) const { return k.Serialize(); }
};
}  // namespace std

namespace kush::khir {

LiveInterval::LiveInterval(int reg)
    : undef_(true),
      start_(-1),
      end_(-1),
      value_(khir::Value(-1)),
      type_(static_cast<khir::Type>(-1)),
      register_(reg),
      spill_cost_(INT32_MAX) {}

LiveInterval::LiveInterval(khir::Value v, khir::Type t)
    : undef_(true),
      start_(-1),
      end_(-1),
      value_(v),
      type_(t),
      register_(-1),
      spill_cost_(0) {}

void LiveInterval::Extend(int x) {
  assert(x >= 0);
  undef_ = false;

  if (start_ == -1 || x < start_) {
    start_ = x;
  }

  if (end_ == -1 || x > end_) {
    end_ = x;
  }
}

void LiveInterval::ChangeToPrecolored(int reg) {
  value_ = khir::Value(-1);
  type_ = static_cast<khir::Type>(-1);
  register_ = reg;
  spill_cost_ = INT32_MAX;
}

void LiveInterval::UpdateSpillCostWithUse(int loop_depth) {
  if (IsPrecolored()) {
    return;
  }

  double add_factor = 1.5 * pow(10, loop_depth);
  int to_add = add_factor >= INT32_MAX ? INT32_MAX : add_factor;

  if (INT32_MAX - to_add < spill_cost_) {
    spill_cost_ = INT32_MAX;
  } else {
    spill_cost_ += to_add;
  }
}

int LiveInterval::Start() const { return start_; }

int LiveInterval::End() const { return end_; }

int LiveInterval::SpillCost() const { return spill_cost_; }

bool LiveInterval::Undef() const { return undef_; }

bool LiveInterval::operator==(const LiveInterval& rhs) {
  return undef_ == rhs.undef_ && start_ == rhs.start_ && end_ == rhs.end_ &&
         value_.Serialize() == rhs.value_.Serialize() &&
         type_.GetID() == rhs.type_.GetID() && register_ == rhs.register_;
}

khir::Type LiveInterval::Type() const {
  if (IsPrecolored()) {
    throw std::runtime_error("Not a virtual live interval");
  }
  return type_;
}

khir::Value LiveInterval::Value() const {
  if (IsPrecolored()) {
    throw std::runtime_error("Not a virtual live interval");
  }
  return value_;
}

int LiveInterval::PrecoloredRegister() const {
  if (IsPrecolored()) {
    return register_;
  }

  throw std::runtime_error("Not a precolored live interval");
}

bool LiveInterval::IsPrecolored() const { return register_ >= 0; }

bool IsGep(Value v, const std::vector<uint64_t>& instructions) {
  if (v.IsConstantGlobal()) {
    return false;
  }
  auto opcode =
      OpcodeFrom(GenericInstructionReader(instructions[v.GetIdx()]).Opcode());
  return opcode == Opcode::GEP_STATIC || opcode == Opcode::GEP_DYNAMIC;
}

bool IsPhi(Value v, const std::vector<uint64_t>& instructions) {
  if (v.IsConstantGlobal()) {
    return false;
  }

  return OpcodeFrom(
             GenericInstructionReader(instructions[v.GetIdx()]).Opcode()) ==
         Opcode::PHI;
}

std::vector<Value> GetReadValuesGEP(int instr_idx,
                                    const std::vector<uint64_t>& instrs) {
  auto opcode =
      OpcodeFrom(GenericInstructionReader(instrs[instr_idx]).Opcode());

  switch (opcode) {
    case Opcode::GEP_STATIC: {
      Type2InstructionReader reader(instrs[instr_idx - 1]);
      if (OpcodeFrom(reader.Opcode()) != Opcode::GEP_STATIC_OFFSET) {
        throw std::runtime_error("Invalid GEP_STATIC_OFFSET");
      }

      auto ptr = Value(reader.Arg0());

      std::vector<Value> result;
      if (!ptr.IsConstantGlobal()) {
        result.push_back(ptr);
      }
      return result;
    }

    case Opcode::GEP_DYNAMIC: {
      Type3InstructionReader reader1(instrs[instr_idx]);
      Type2InstructionReader reader2(instrs[instr_idx - 1]);
      if (OpcodeFrom(reader2.Opcode()) != Opcode::GEP_DYNAMIC_OFFSET) {
        throw std::runtime_error("Invalid GEP_DYNAMIC_OFFSET");
      }

      auto ptr = Value(reader1.Arg());
      auto index_offset = Value(reader2.Arg0());
      auto offset = Value(reader2.Arg1());

      std::vector<Value> result;
      if (!ptr.IsConstantGlobal()) {
        result.push_back(ptr);
      }
      if (!index_offset.IsConstantGlobal()) {
        result.push_back(index_offset);
      }
      if (!offset.IsConstantGlobal()) {
        result.push_back(offset);
      }
      return result;
    }

    default:
      throw std::runtime_error("Not a GEP");
  }
}

Type TypeOf(uint64_t instr, const std::vector<uint64_t>& instrs,
            const TypeManager& manager) {
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case Opcode::I1_AND:
    case Opcode::I1_OR:
    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I1_LNOT:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE:
    case Opcode::PTR_CMP_NULLPTR:
    case Opcode::I1_LOAD:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8:
      return manager.I1Type();

    case Opcode::MASK_TO_PERMUTE:
      return manager.I32Vec8PtrType();

    case Opcode::I64_POPCOUNT:
    case Opcode::I1_VEC8_MASK_EXTRACT:
      return manager.I64Type();

    case Opcode::I32_VEC8_CMP_EQ:
    case Opcode::I32_VEC8_CMP_NE:
    case Opcode::I32_VEC8_CMP_LT:
    case Opcode::I32_VEC8_CMP_LE:
    case Opcode::I32_VEC8_CMP_GT:
    case Opcode::I32_VEC8_CMP_GE:
    case Opcode::I1_VEC8_AND:
    case Opcode::I1_VEC8_OR:
    case Opcode::I1_VEC8_NOT:
      return manager.I1Vec8Type();

    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_LOAD:
    case Opcode::I1_ZEXT_I8:
      return manager.I8Type();

    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_LOAD:
    case Opcode::I64_TRUNC_I16:
      return manager.I16Type();

    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_LOAD:
    case Opcode::I64_TRUNC_I32:
      return manager.I32Type();

    case Opcode::I32_VEC8_ADD:
    case Opcode::I32_CONV_I32_VEC8:
    case Opcode::I32_VEC8_LOAD:
    case Opcode::I32_VEC8_PERMUTE:
      return manager.I32Vec8Type();

    case Opcode::I64_LSHIFT:
    case Opcode::I64_RSHIFT:
    case Opcode::I64_AND:
    case Opcode::I64_XOR:
    case Opcode::I64_OR:
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::F64_CONV_I64:
    case Opcode::I64_LOAD:
      return manager.I64Type();

    case Opcode::F64_ADD:
    case Opcode::F64_MUL:
    case Opcode::F64_SUB:
    case Opcode::F64_DIV:
    case Opcode::I8_CONV_F64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_CONV_F64:
    case Opcode::I64_CONV_F64:
    case Opcode::F64_LOAD:
      return manager.F64Type();

    case Opcode::RETURN:
    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE:
    case Opcode::I32_VEC8_MASK_STORE:
    case Opcode::RETURN_VALUE:
    case Opcode::CONDBR:
    case Opcode::BR:
      return manager.VoidType();

    case Opcode::CALL_INDIRECT:
    case Opcode::PHI:
    case Opcode::CALL:
    case Opcode::PTR_LOAD:
    case Opcode::PTR_CAST:
    case Opcode::GEP_STATIC:
    case Opcode::GEP_DYNAMIC:
    case Opcode::FUNC_ARG:
      return static_cast<Type>(Type3InstructionReader(instr).TypeID());

    case Opcode::GEP_STATIC_OFFSET:
    case Opcode::GEP_DYNAMIC_OFFSET:
    case Opcode::PHI_MEMBER:
    case Opcode::CALL_ARG:
    case Opcode::I32_VEC8_MASK_STORE_INFO:
      return manager.VoidType();
  }
}

std::optional<Value> GetWrittenValue(int instr_idx,
                                     const std::vector<uint64_t>& instrs,
                                     const std::vector<bool>& materialize_gep,
                                     const TypeManager& manager) {
  auto instr = instrs[instr_idx];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case Opcode::I1_AND:
    case Opcode::I1_OR:
    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE:
    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE:
    case Opcode::I32_VEC8_CMP_EQ:
    case Opcode::I32_VEC8_CMP_NE:
    case Opcode::I32_VEC8_CMP_LT:
    case Opcode::I32_VEC8_CMP_LE:
    case Opcode::I32_VEC8_CMP_GT:
    case Opcode::I32_VEC8_CMP_GE:
    case Opcode::I32_VEC8_PERMUTE:
    case Opcode::I32_VEC8_ADD:
    case Opcode::I1_VEC8_AND:
    case Opcode::I1_VEC8_OR:
    case Opcode::I1_VEC8_NOT:
    case Opcode::MASK_TO_PERMUTE:
    case Opcode::I64_POPCOUNT:
    case Opcode::I1_VEC8_MASK_EXTRACT:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8:
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I64_LSHIFT:
    case Opcode::I64_RSHIFT:
    case Opcode::I64_AND:
    case Opcode::I64_OR:
    case Opcode::I64_XOR:
    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_ADD:
    case Opcode::F64_MUL:
    case Opcode::F64_SUB:
    case Opcode::F64_DIV:
    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE:
    case Opcode::I1_LNOT:
    case Opcode::I1_ZEXT_I8:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I8_CONV_F64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::I32_CONV_F64:
    case Opcode::I32_CONV_I32_VEC8:
    case Opcode::I64_CONV_F64:
    case Opcode::I64_TRUNC_I16:
    case Opcode::I64_TRUNC_I32:
    case Opcode::F64_CONV_I64:
    case Opcode::I1_LOAD:
    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I32_VEC8_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD:
    case Opcode::FUNC_ARG:
    case Opcode::PTR_CAST:
    case Opcode::PTR_LOAD:
    case Opcode::CALL_ARG:
    case Opcode::PTR_CMP_NULLPTR: {
      return Value(instr_idx);
    }

    case Opcode::CALL:
    case Opcode::CALL_INDIRECT: {
      if (!manager.IsVoid(
              static_cast<Type>(Type3InstructionReader(instr).TypeID()))) {
        return Value(instr_idx);
      }
      return std::nullopt;
    }

    case Opcode::PHI_MEMBER: {
      // writes the phi here
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      return Value(v0.GetIdx());
    }

    case Opcode::GEP_DYNAMIC:
    case Opcode::GEP_STATIC: {
      if (materialize_gep[instr_idx]) {
        return Value(instr_idx);
      }
      return std::nullopt;
    }

    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE:
    case Opcode::I32_VEC8_MASK_STORE:
    case Opcode::I32_VEC8_MASK_STORE_INFO:
    case Opcode::GEP_STATIC_OFFSET:
    case Opcode::GEP_DYNAMIC_OFFSET:
    case Opcode::BR:
    case Opcode::CONDBR:
    case Opcode::RETURN:
    case Opcode::RETURN_VALUE:
    case Opcode::PHI: {
      return std::nullopt;
    }
  }
}

std::vector<Value> GetReadValues(int instr_idx, int seg_start, int seg_end,
                                 const std::vector<bool>& materialize_gep,
                                 const std::vector<uint64_t>& instrs) {
  auto instr = instrs[instr_idx];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case Opcode::I1_AND:
    case Opcode::I1_OR:
    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE:
    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE:
    case Opcode::I32_VEC8_CMP_EQ:
    case Opcode::I32_VEC8_CMP_NE:
    case Opcode::I32_VEC8_CMP_LT:
    case Opcode::I32_VEC8_CMP_LE:
    case Opcode::I32_VEC8_CMP_GT:
    case Opcode::I32_VEC8_CMP_GE:
    case Opcode::I32_VEC8_ADD:
    case Opcode::I1_VEC8_AND:
    case Opcode::I32_VEC8_PERMUTE:
    case Opcode::I1_VEC8_OR:
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I64_LSHIFT:
    case Opcode::I64_RSHIFT:
    case Opcode::I64_AND:
    case Opcode::I64_OR:
    case Opcode::I64_XOR:
    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_ADD:
    case Opcode::F64_MUL:
    case Opcode::F64_SUB:
    case Opcode::F64_DIV:
    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      std::vector<Value> result;
      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      if (!v1.IsConstantGlobal()) {
        result.push_back(v1);
      }

      return result;
    }

    case Opcode::PHI_MEMBER: {
      Type2InstructionReader reader(instr);
      Value v1(reader.Arg1());

      std::vector<Value> result;

      if (IsGep(v1, instrs) && !materialize_gep[v1.GetIdx()]) {
        throw std::runtime_error("Requried materialized GEP into PHI_MEMBER.");
      }

      if (!v1.IsConstantGlobal()) {
        result.push_back(v1);
      }

      return result;
    }

    case Opcode::I1_LNOT:
    case Opcode::MASK_TO_PERMUTE:
    case Opcode::I64_POPCOUNT:
    case Opcode::I1_VEC8_MASK_EXTRACT:
    case Opcode::I1_VEC8_NOT:
    case Opcode::I1_ZEXT_I8:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I8_CONV_F64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::I32_CONV_F64:
    case Opcode::I32_CONV_I32_VEC8:
    case Opcode::I64_CONV_F64:
    case Opcode::I64_TRUNC_I16:
    case Opcode::I64_TRUNC_I32:
    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      std::vector<Value> result;
      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::PTR_CMP_NULLPTR: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      std::vector<Value> result;
      if (IsGep(v0, instrs) && !materialize_gep[v0.GetIdx()]) {
        throw std::runtime_error(
            "Requried materialized GEP into PTR_CMP_NULLPTR.");
      }

      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::I1_LOAD:
    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I32_VEC8_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      std::vector<Value> result;
      if (IsGep(v, instrs) && !materialize_gep[v.GetIdx()]) {
        result = GetReadValuesGEP(v.GetIdx(), instrs);
      } else if (!v.IsConstantGlobal()) {
        result.push_back(v);
      }
      return result;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      Value v0(reader.Arg());

      std::vector<Value> result;
      if (IsGep(v0, instrs) && !materialize_gep[v0.GetIdx()]) {
        throw std::runtime_error("Required materialized GEP into PTR_CAST.");
      }
      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      Value v0(reader.Arg());

      std::vector<Value> result;
      if (IsGep(v0, instrs) && !materialize_gep[v0.GetIdx()]) {
        result = GetReadValuesGEP(v0.GetIdx(), instrs);
      } else if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      std::vector<Value> result;
      if (IsGep(v0, instrs) && !materialize_gep[v0.GetIdx()]) {
        result = GetReadValuesGEP(v0.GetIdx(), instrs);
      } else if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      if (IsGep(v1, instrs) && !materialize_gep[v1.GetIdx()]) {
        throw std::runtime_error("Required materialized GEP into XXX_STORE.");
      }
      if (!v1.IsConstantGlobal()) {
        result.push_back(v1);
      }

      return result;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      Value v0(reader.Arg());

      std::vector<Value> result;
      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::RETURN_VALUE:
    case Opcode::CALL_ARG: {
      Type3InstructionReader reader(instr);
      Value v0(reader.Arg());

      std::vector<Value> result;
      if (IsGep(v0, instrs) && !materialize_gep[v0.GetIdx()]) {
        throw std::runtime_error(
            "Required materialized GEP into RETURN_VALUE/CALL_ARG.");
      }

      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::CALL_INDIRECT:
    case Opcode::CALL: {  // reads all call args
      std::vector<Value> result;

      if (opcode == Opcode::CALL_INDIRECT) {
        Type3InstructionReader reader(instr);
        Value v0(reader.Arg());

        if (IsGep(v0, instrs) && !materialize_gep[v0.GetIdx()]) {
          throw std::runtime_error(
              "Required materialized GEP into CALL_INDIRECT.");
        }

        if (!v0.IsConstantGlobal()) {
          result.push_back(v0);
        }
      }

      for (int j = instr_idx - 1; j >= seg_start; j--) {
        auto opcode = OpcodeFrom(GenericInstructionReader(instrs[j]).Opcode());
        if (opcode == Opcode::CALL_ARG) {
          result.emplace_back(j);
        } else {
          break;
        }
      }
      return result;
    }

    case Opcode::GEP_DYNAMIC:
    case Opcode::GEP_STATIC: {
      if (materialize_gep[instr_idx]) {
        return GetReadValuesGEP(instr_idx, instrs);
      }
      return {};
    }

    case Opcode::I32_VEC8_MASK_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      std::vector<Value> result;
      if (IsGep(v0, instrs) && !materialize_gep[v0.GetIdx()]) {
        result = GetReadValuesGEP(v0.GetIdx(), instrs);
      } else if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      if (IsGep(v1, instrs) && !materialize_gep[v1.GetIdx()]) {
        throw std::runtime_error("Required materialized GEP into XXX_STORE.");
      }
      if (!v1.IsConstantGlobal()) {
        result.push_back(v1);
      }

      Type2InstructionReader reader_info(instrs[instr_idx - 1]);
      Value v2(reader_info.Arg0());
      if (!v2.IsConstantGlobal()) {
        result.push_back(v2);
      }

      return result;
    }

    case Opcode::RETURN:
    case Opcode::BR:
    case Opcode::FUNC_ARG:
    case Opcode::GEP_STATIC_OFFSET:
    case Opcode::GEP_DYNAMIC_OFFSET:
    case Opcode::PHI:
    case Opcode::I32_VEC8_MASK_STORE_INFO:
      return {};
  }
}

void AddExternalUses(absl::flat_hash_set<Value>& values,
                     const std::vector<uint64_t>& instrs,
                     const std::vector<bool>& gep_materialize,
                     const BasicBlock& bb, const TypeManager& manager) {
  absl::flat_hash_set<Value> written_to;

  for (const auto& [seg_start, seg_end] : bb.Segments()) {
    for (int i = seg_start; i <= seg_end; i++) {
      for (auto v :
           GetReadValues(i, seg_start, seg_end, gep_materialize, instrs)) {
        if (!written_to.contains(v)) {
          values.insert(v);
        }
      }

      auto written_value = GetWrittenValue(i, instrs, gep_materialize, manager);
      if (written_value.has_value()) {
        written_to.insert(written_value.value());
      }
    }
  }
}

void AddDefs(absl::flat_hash_set<Value>& values,
             const std::vector<uint64_t>& instrs,
             const std::vector<bool>& gep_materialize,
             const TypeManager& manager, const BasicBlock& bb) {
  for (const auto& [seg_start, seg_end] : bb.Segments()) {
    for (int i = seg_start; i <= seg_end; i++) {
      auto v = GetWrittenValue(i, instrs, gep_materialize, manager);
      if (v.has_value()) {
        values.insert(v.value());
      }
    }
  }
}

absl::flat_hash_set<Value> Union(
    const std::vector<int>& successors,
    const std::vector<absl::flat_hash_set<Value>>& live_in) {
  absl::flat_hash_set<Value> out;
  for (int i : successors) {
    out.insert(live_in[i].begin(), live_in[i].end());
  }
  return out;
}

std::vector<LiveInterval> ComputeLiveIntervals(
    const Function& func, const std::vector<bool>& gep_materialize,
    const TypeManager& manager) {
  const auto& bb = func.BasicBlocks();
  const auto& instrs = func.Instrs();

  std::vector<absl::flat_hash_set<Value>> live_in(bb.size());
  std::vector<absl::flat_hash_set<Value>> uses(bb.size());
  std::vector<absl::flat_hash_set<Value>> defs(bb.size());
  std::stack<int> worklist;
  for (int i = 0; i < bb.size(); i++) {
    AddExternalUses(live_in[i], instrs, gep_materialize, bb[i], manager);
    AddDefs(defs[i], instrs, gep_materialize, manager, bb[i]);
    worklist.push(i);
  }

  while (!worklist.empty()) {
    int curr_block = worklist.top();
    worklist.pop();

    auto live_out = Union(bb[curr_block].Successors(), live_in);

    auto initial_size = live_in[curr_block].size();
    for (auto v : live_out) {
      if (!defs[curr_block].contains(v)) {
        live_in[curr_block].insert(v);
      }
    }

    if (live_in[curr_block].size() != initial_size) {
      for (const int pred : bb[curr_block].Predecessors()) {
        worklist.push(pred);
      }
    }
  }

  std::vector<LiveInterval> live_intervals;
  live_intervals.reserve(instrs.size());
  for (int i = 0; i < instrs.size(); i++) {
    auto v = Value(i);
    live_intervals.emplace_back(v, TypeOf(instrs[i], instrs, manager));
  }

  absl::flat_hash_map<std::tuple<int, int>, int> instr_map;
  {
    int total_past_instrs = 0;
    for (int bb_idx = 0; bb_idx < bb.size(); bb_idx++) {
      for (const auto& [seg_start, seg_end] : bb[bb_idx].Segments()) {
        for (int i = seg_start; i <= seg_end; i++) {
          instr_map[{bb_idx, i}] = total_past_instrs++;
        }
      }
    }
  }

  for (int bb_idx = 0; bb_idx < bb.size(); bb_idx++) {
    auto currently_live = Union(bb[bb_idx].Successors(), live_in);

    for (int seg_idx = bb[bb_idx].Segments().size() - 1; seg_idx >= 0;
         seg_idx--) {
      auto [seg_start, seg_end] = bb[bb_idx].Segments()[seg_idx];
      for (int i = seg_end; i >= seg_start; i--) {
        for (auto v : currently_live) {
          live_intervals[v.GetIdx()].Extend(instr_map.at({bb_idx, i}));
        }

        auto written_value =
            GetWrittenValue(i, instrs, gep_materialize, manager);
        if (written_value.has_value()) {
          currently_live.erase(written_value.value());
        }
        for (auto v :
             GetReadValues(i, seg_start, seg_end, gep_materialize, instrs)) {
          currently_live.insert(v);
        }

        for (auto v : currently_live) {
          live_intervals[v.GetIdx()].Extend(instr_map.at({bb_idx, i}));
        }
      }
    }

    if (currently_live != live_in[bb_idx]) {
      throw std::runtime_error("Invalid liveness");
    }
  }

  // Create an interval for each store instruction
  for (int bb_idx = 0; bb_idx < bb.size(); bb_idx++) {
    for (const auto& [seg_start, seg_end] : bb[bb_idx].Segments()) {
      for (int i = seg_start; i <= seg_end; i++) {
        auto i_opcode =
            OpcodeFrom(GenericInstructionReader(instrs[i]).Opcode());
        switch (i_opcode) {
          case Opcode::I8_STORE:
          case Opcode::I16_STORE:
          case Opcode::I32_STORE:
          case Opcode::I64_STORE:
          case Opcode::F64_STORE:
          case Opcode::PTR_STORE:
          case Opcode::I32_VEC8_MASK_STORE: {
            live_intervals[i].Extend(instr_map.at({bb_idx, i}));
            break;
          }

          default:
            break;
        }
      }
    }
  }

  // Create a precolored interval for each function argument
  std::vector<int> normal_arg_reg{
      GPRegister::RDI.Id(), GPRegister::RSI.Id(), GPRegister::RDX.Id(),
      GPRegister::RCX.Id(), GPRegister::R8.Id(),  GPRegister::R9.Id(),
  };
  std::vector<int> fp_arg_reg{
      VRegister::M0.Id(), VRegister::M1.Id(), VRegister::M2.Id(),
      VRegister::M3.Id(), VRegister::M4.Id(), VRegister::M5.Id(),
      VRegister::M6.Id(), VRegister::M7.Id(),
  };

  // Caller save in addition to above arg regs
  std::vector<int> normal_caller_save{
      GPRegister::R10.Id(),
      GPRegister::R11.Id(),
  };
  std::vector<int> fp_caller_save{
      VRegister::M8.Id(),  VRegister::M9.Id(),  VRegister::M10.Id(),
      VRegister::M11.Id(), VRegister::M12.Id(), VRegister::M13.Id(),
      VRegister::M14.Id(),
  };

  for (int i = 0, normal_arg_ctr = 0, fp_arg_ctr = 0; i < instrs.size(); i++) {
    auto opcode = OpcodeFrom(GenericInstructionReader(instrs[i]).Opcode());
    if (opcode != Opcode::FUNC_ARG) {
      break;
    }

    Type3InstructionReader reader(instrs[i]);
    auto type = static_cast<Type>(reader.TypeID());
    if (manager.IsF64Type(type)) {
      auto reg = fp_arg_reg[fp_arg_ctr++];
      LiveInterval interval(reg);
      interval.Extend(instr_map.at({0, 0}));
      interval.Extend(instr_map.at({0, i}));
      live_intervals.push_back(interval);
    } else {
      auto reg = normal_arg_reg[normal_arg_ctr++];
      LiveInterval interval(reg);
      interval.Extend(instr_map.at({0, 0}));
      interval.Extend(instr_map.at({0, i}));
      live_intervals.push_back(interval);
    }
  }

  // Create a precolored interval for all clobbered registers
  int normal_arg_ctr = 0;
  int fp_arg_ctr = 0;
  for (int bb_idx = 0; bb_idx < bb.size(); bb_idx++) {
    for (const auto& [seg_start, seg_end] : bb[bb_idx].Segments()) {
      for (int i = seg_start; i <= seg_end; i++) {
        auto i_opcode =
            OpcodeFrom(GenericInstructionReader(instrs[i]).Opcode());
        auto i_type = TypeOf(instrs[i], instrs, manager);
        switch (i_opcode) {
          case Opcode::CALL_ARG: {
            if (manager.IsF64Type(i_type)) {
              // Passing argument by stack so just move on.
              if (fp_arg_ctr >= fp_arg_reg.size()) {
                fp_arg_ctr++;
              } else {
                int reg = fp_arg_reg[fp_arg_ctr++];
                live_intervals[i].ChangeToPrecolored(reg);
              }
            } else {
              // Passing argument by stack so just move on.
              if (normal_arg_ctr >= normal_arg_reg.size()) {
                normal_arg_ctr++;
              } else {
                int reg = normal_arg_reg[normal_arg_ctr++];
                live_intervals[i].ChangeToPrecolored(reg);
              }
            }
            break;
          }

          case Opcode::CALL_INDIRECT:
          case Opcode::CALL: {
            // add in live intervals for all remaining arg regs since they are
            // clobbered
            while (normal_arg_ctr < normal_arg_reg.size()) {
              int reg = normal_arg_reg[normal_arg_ctr++];
              LiveInterval interval(reg);
              interval.Extend(instr_map.at({bb_idx, i}));
              live_intervals.push_back(interval);
            }

            while (fp_arg_ctr < fp_arg_reg.size()) {
              int reg = fp_arg_reg[fp_arg_ctr++];
              LiveInterval interval(reg);
              interval.Extend(instr_map.at({bb_idx, i}));
              live_intervals.push_back(interval);
            }

            // clobber remaining caller saved registers
            for (int reg : normal_caller_save) {
              LiveInterval interval(reg);
              interval.Extend(instr_map.at({bb_idx, i}));
              live_intervals.push_back(interval);
            }

            for (int reg : fp_caller_save) {
              LiveInterval interval(reg);
              interval.Extend(instr_map.at({bb_idx, i}));
              live_intervals.push_back(interval);
            }

            // reset arg counters
            normal_arg_ctr = 0;
            fp_arg_ctr = 0;
            break;
          }

          default:
            break;
        }
      }
    }
  }

  // update cost of the live intervals
  auto loop_tree = FindLoops(bb);
  // compute depth in loop tree
  std::vector<int> loop_depth(bb.size(), 0);
  {
    // find all nodes that are roots in the loop forest
    std::vector<bool> is_root(bb.size(), true);
    for (const auto& x : loop_tree) {
      for (int y : x) {
        is_root[y] = false;
      }
    }

    // DFS from each root marking loop depth
    std::function<void(int, int)> dfs = [&](int curr, int curr_depth) {
      loop_depth[curr] = curr_depth;
      for (int next : loop_tree[curr]) {
        dfs(next, curr_depth + 1);
      }
    };

    for (int i = 0; i < bb.size(); i++) {
      if (is_root[i]) {
        dfs(i, 0);
      }
    }
  }
  // update spill costs
  for (int bb_idx = 0; bb_idx < bb.size(); bb_idx++) {
    for (const auto& [seg_start, seg_end] : bb[bb_idx].Segments()) {
      for (int i = seg_start; i <= seg_end; i++) {
        auto written_value =
            GetWrittenValue(i, instrs, gep_materialize, manager);
        if (written_value.has_value()) {
          live_intervals[written_value.value().GetIdx()].UpdateSpillCostWithUse(
              loop_depth[bb_idx]);
        }
        for (auto v :
             GetReadValues(i, seg_start, seg_end, gep_materialize, instrs)) {
          live_intervals[v.GetIdx()].UpdateSpillCostWithUse(loop_depth[bb_idx]);
        }
      }
    }
  }

  std::vector<LiveInterval> outputs;
  outputs.reserve(instrs.size());
  for (const auto& l : live_intervals) {
    if (!l.Undef()) {
      outputs.push_back(l);
    }
  }
  return outputs;
}

}  // namespace kush::khir