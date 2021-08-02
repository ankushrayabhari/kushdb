#include "khir/asm/live_intervals.h"

#include <iostream>
#include <stack>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "khir/instruction.h"
#include "khir/program_builder.h"
#include "util/union_find.h"

namespace kush::khir {

LiveInterval::LiveInterval(int reg)
    : undef_(true),
      start_bb_(-1),
      end_bb_(-1),
      start_idx_(-1),
      end_idx_(-1),
      value_(khir::Value(-1)),
      type_(static_cast<khir::Type>(-1)),
      register_(reg) {}

LiveInterval::LiveInterval(khir::Value v, khir::Type t)
    : undef_(true),
      start_bb_(-1),
      end_bb_(-1),
      start_idx_(-1),
      end_idx_(-1),
      value_(v),
      type_(t),
      register_(-1) {}

void LiveInterval::Extend(int bb, int idx) {
  assert(bb >= 0);
  assert(idx >= 0);
  undef_ = false;

  if (start_bb_ == -1) {
    start_bb_ = bb;
    start_idx_ = idx;
  } else if (start_bb_ == bb && idx < start_idx_) {
    start_idx_ = idx;
  } else if (bb < start_bb_) {
    start_bb_ = bb;
    start_idx_ = idx;
  }

  if (end_bb_ == -1) {
    end_bb_ = bb;
    end_idx_ = idx;
  } else if (end_bb_ == bb && idx > end_idx_) {
    end_idx_ = idx;
  } else if (bb > end_bb_) {
    end_bb_ = bb;
    end_idx_ = idx;
  }
}

void LiveInterval::ChangeToPrecolored(int reg) {
  value_ = khir::Value(-1);
  type_ = static_cast<khir::Type>(-1);
  register_ = reg;
}

int LiveInterval::StartBB() const { return start_bb_; }

int LiveInterval::EndBB() const { return end_bb_; }

int LiveInterval::StartIdx() const { return start_idx_; }

int LiveInterval::EndIdx() const { return end_idx_; }

bool LiveInterval::Undef() const { return undef_; }

bool LiveInterval::operator==(const LiveInterval& rhs) {
  return undef_ == rhs.undef_ && start_bb_ == rhs.start_bb_ &&
         end_bb_ == rhs.end_bb_ && start_idx_ == rhs.start_idx_ &&
         end_idx_ == rhs.end_idx_ &&
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

std::vector<std::vector<int>> ComputeBackedges(const Function& func) {
  // TODO: replace with linear time dominator algorithm
  const auto& bb = func.BasicBlocks();
  const auto& bb_succ = func.BasicBlockSuccessors();
  const auto& bb_pred = func.BasicBlockPredecessors();
  std::vector<absl::flat_hash_set<int>> dom(bb.size());
  dom[0].insert(0);
  for (int i = 1; i < bb.size(); i++) {
    for (int j = 0; j < bb.size(); j++) {
      dom[i].insert(j);
    }
  }

  bool changes = true;
  while (changes) {
    changes = false;

    for (int i = 1; i < bb.size(); i++) {
      int initial_size = dom[i].size();

      dom[i].clear();
      const auto& pred = bb_pred[i];
      if (pred.size() > 0) {
        dom[i] =
            absl::flat_hash_set<int>(dom[pred[0]].begin(), dom[pred[0]].end());
        for (int k = 1; k < pred.size(); k++) {
          for (int x : dom[pred[k]]) {
            if (!dom[i].contains(x)) {
              dom[i].erase(x);
            }
          }
        }
      }
      dom[i].insert(i);

      changes = changes || dom[i].size() != initial_size;
    }
  }

  std::vector<std::vector<int>> backedges(bb.size());
  for (int i = 0; i < bb.size(); i++) {
    for (int x : bb_succ[i]) {
      if (dom[i].contains(x)) {
        backedges[x].push_back(i);
      }
    }
  }

  return backedges;
}

void Collapse(std::vector<int>& loop_parent, std::vector<int>& union_find,
              const absl::flat_hash_set<int>& loop_body, int loop_header) {
  for (int z : loop_body) {
    loop_parent[z] = loop_header;
    util::UnionFind::Union(union_find, z, loop_header);
  }
}

void FindLoopHelper(int curr, const std::vector<std::vector<int>>& bb_succ,
                    const std::vector<std::vector<int>>& bb_pred,
                    const std::vector<std::vector<int>>& backedges,
                    std::vector<bool>& visited, std::vector<int>& union_find,
                    std::vector<int>& loop_parent,
                    std::vector<bool>& is_loop_header) {
  // DFS
  for (auto succ : bb_succ[curr]) {
    if (!visited[succ]) {
      visited[succ] = true;
      FindLoopHelper(succ, bb_succ, bb_pred, backedges, visited, union_find,
                     loop_parent, is_loop_header);
    }
  }

  absl::flat_hash_set<int> loop_body;
  absl::flat_hash_set<int> work_list;
  for (int y : backedges[curr]) {
    work_list.insert(util::UnionFind::Find(union_find, y));
  }
  work_list.erase(curr);

  while (!work_list.empty()) {
    int y = *work_list.begin();
    work_list.erase(y);

    loop_body.insert(y);

    for (int z : bb_pred[y]) {
      if (std::find(backedges[y].begin(), backedges[y].end(), z) ==
          backedges[y].end()) {
        auto repr = util::UnionFind::Find(union_find, z);
        if (repr != curr && !loop_body.contains(repr) &&
            !work_list.contains(repr)) {
          work_list.insert(repr);
        }
      }
    }
  }

  if (!loop_body.empty()) {
    int exit_block = -1;
    for (int x : bb_succ[curr]) {
      auto repr = util::UnionFind::Find(union_find, x);
      if (loop_body.contains(x)) {
        continue;
      }
      exit_block = repr;
      break;
    }
    assert(exit_block >= 0);

    for (int y : loop_body) {
      for (int z : bb_succ[y]) {
        auto repr = util::UnionFind::Find(union_find, z);
        if (repr != exit_block && z != curr && !loop_body.contains(repr)) {
          work_list.insert(repr);
        }
      }
    }

    while (!work_list.empty()) {
      int y = *work_list.begin();
      work_list.erase(y);

      // all predecessor blocks are in the loop, then it's part of the loop
      for (int x : bb_pred[y]) {
        auto repr = util::UnionFind::Find(union_find, x);
        if (!loop_body.contains(repr)) {
          throw std::runtime_error("not all predecessors are in loop.");
        }
      }

      loop_body.insert(y);

      for (int z : bb_succ[y]) {
        auto repr = util::UnionFind::Find(union_find, z);
        if (repr != exit_block && z != curr && !loop_body.contains(repr)) {
          work_list.insert(repr);
        }
      }
    }

    is_loop_header[curr] = true;
    Collapse(loop_parent, union_find, loop_body, curr);
  }
}

std::pair<std::vector<int>, std::vector<bool>> FindLoops(const Function& func) {
  const auto& bb = func.BasicBlocks();
  const auto& bb_succ = func.BasicBlockSuccessors();
  const auto& bb_pred = func.BasicBlockPredecessors();
  auto backedges = ComputeBackedges(func);

  // Find all loops
  std::vector<int> union_find(bb.size());
  for (int i = 0; i < bb.size(); i++) {
    union_find[i] = i;
  }

  std::vector<int> loop_parent(bb.size(), 0);
  std::vector<bool> visited(bb.size(), false);
  std::vector<bool> is_loop_header(bb.size(), false);
  FindLoopHelper(0, bb_succ, bb_pred, backedges, visited, union_find,
                 loop_parent, is_loop_header);
  return {loop_parent, is_loop_header};
}

void LabelBlocks(int curr, const std::vector<std::vector<int>>& bb_succ,
                 std::stack<int>& output, std::vector<bool>& visited,
                 const std::vector<int>& loop_parent) {
  visited[curr] = true;

  for (auto succ : bb_succ[curr]) {
    if (loop_parent[succ] == curr) {
      continue;
    }

    if (!visited[succ]) {
      LabelBlocks(succ, bb_succ, output, visited, loop_parent);
    }
  }

  for (auto succ : bb_succ[curr]) {
    if (!visited[succ]) {
      LabelBlocks(succ, bb_succ, output, visited, loop_parent);
    }
  }

  output.push(curr);
}

std::pair<std::vector<int>, std::vector<int>> LabelBlocks(
    const Function& func, const std::vector<int>& loop_parent) {
  const auto& bb = func.BasicBlocks();
  const auto& bb_succ = func.BasicBlockSuccessors();

  std::stack<int> temp;
  std::vector<bool> visited(bb.size(), false);
  visited[0] = true;
  LabelBlocks(0, bb_succ, temp, visited, loop_parent);

  std::vector<int> label(bb.size(), -1);
  std::vector<int> order(bb.size(), -1);
  int i = 0;
  while (!temp.empty()) {
    order[i] = temp.top();
    label[temp.top()] = i++;
    temp.pop();
  }

  return {label, order};
}

bool IsGep(khir::Value v, const std::vector<uint64_t>& instructions) {
  if (v.IsConstantGlobal()) {
    return false;
  }
  return OpcodeFrom(
             GenericInstructionReader(instructions[v.GetIdx()]).Opcode()) ==
         Opcode::GEP;
}

khir::Value Gep(khir::Value v, const std::vector<uint64_t>& instructions) {
  Type3InstructionReader gep_reader(instructions[v.GetIdx()]);
  if (OpcodeFrom(gep_reader.Opcode()) != Opcode::GEP) {
    throw std::runtime_error("Invalid GEP");
  }

  auto gep_offset_instr = instructions[khir::Value(gep_reader.Arg()).GetIdx()];
  Type2InstructionReader gep_offset_reader(gep_offset_instr);
  if (OpcodeFrom(gep_offset_reader.Opcode()) != Opcode::GEP_OFFSET) {
    throw std::runtime_error("Invalid GEP Offset");
  }

  return khir::Value(gep_offset_reader.Arg0());
}

std::vector<Value> ComputeReadValues(uint64_t instr,
                                     const std::vector<uint64_t>& instrs) {
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
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
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
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
    case Opcode::F64_CMP_GE: {
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

      if (!v1.IsConstantGlobal()) {
        result.push_back(v1);
      }

      return result;
    }

    case Opcode::I1_LNOT:
    case Opcode::I1_ZEXT_I8:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I8_CONV_F64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::I32_CONV_F64:
    case Opcode::I64_CONV_F64:
    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      std::vector<Value> result;
      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      if (IsGep(v, instrs)) {
        v = Gep(v, instrs);
      }

      std::vector<Value> result;
      if (!v.IsConstantGlobal()) {
        result.push_back(v);
      }
      return result;
    }

    case Opcode::PTR_MATERIALIZE:
    case Opcode::PTR_CAST:
    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      Value v0(reader.Arg());

      if (IsGep(v0, instrs)) {
        v0 = Gep(v0, instrs);
      }

      std::vector<Value> result;
      if (!v0.IsConstantGlobal()) {
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

      if (IsGep(v0, instrs)) {
        v0 = Gep(v0, instrs);
      }

      std::vector<Value> result;
      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
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
    case Opcode::CALL_ARG:
    case Opcode::CALL_INDIRECT: {
      Type3InstructionReader reader(instr);
      Value v0(reader.Arg());

      if (IsGep(v0, instrs)) {
        v0 = Gep(v0, instrs);
      }

      std::vector<Value> result;
      if (!v0.IsConstantGlobal()) {
        result.push_back(v0);
      }

      return result;
    }

    case Opcode::RETURN:
    case Opcode::BR:
    case Opcode::FUNC_ARG:
    case Opcode::GEP:
    case Opcode::GEP_OFFSET:
    case Opcode::PHI:
    case Opcode::ALLOCA:
    case Opcode::CALL:  // call arg must be in same bb as call so we are good{
      return {};
  }
}

bool DoesWriteValue(uint64_t instr, const TypeManager& manager) {
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
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
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
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
    case Opcode::I64_CONV_F64:
    case Opcode::F64_CONV_I64:
    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD:
    case Opcode::FUNC_ARG:
    case Opcode::PTR_CAST:
    case Opcode::PTR_MATERIALIZE:
    case Opcode::PTR_LOAD:
    case Opcode::CALL_ARG:
    case Opcode::ALLOCA: {
      return true;
    }

    // Generate intervals for stores since they need an extra register.
    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE: {
      return true;
    }

    case Opcode::GEP:
    case Opcode::GEP_OFFSET:
    case Opcode::PHI_MEMBER:
    case Opcode::RETURN:
    case Opcode::BR:
    case Opcode::CONDBR:
    case Opcode::RETURN_VALUE:
    case Opcode::PHI: {
      return false;
    }

    case Opcode::CALL:  // call arg must be in same bb as call so we are good
    case Opcode::CALL_INDIRECT: {
      return !manager.IsVoid(
          static_cast<Type>(Type3InstructionReader(instr).TypeID()));
    }
  }
}

Type TypeOf(uint64_t instr, const std::vector<uint64_t>& instrs,
            const TypeManager& manager) {
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
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
      return manager.I1Type();

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
      return manager.I16Type();

    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_LOAD:
      return manager.I32Type();

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
    case Opcode::RETURN_VALUE:
    case Opcode::CONDBR:
    case Opcode::BR:
      return manager.VoidType();

    case Opcode::CALL_INDIRECT:
    case Opcode::ALLOCA:
    case Opcode::PHI:
    case Opcode::CALL:
    case Opcode::PTR_LOAD:
    case Opcode::PTR_MATERIALIZE:
    case Opcode::PTR_CAST:
    case Opcode::GEP:
    case Opcode::FUNC_ARG:
      return static_cast<Type>(Type3InstructionReader(instr).TypeID());

    case Opcode::GEP_OFFSET:
    case Opcode::PHI_MEMBER:
    case Opcode::CALL_ARG:
      return manager.VoidType();
  }
}

std::pair<std::vector<int>, std::vector<int>> ComputeLoopDepthEnd(
    const std::vector<int>& loop_parent, const std::vector<bool>& loop_header,
    const std::vector<int>& order) {
  std::vector<int> loop_depth(loop_parent.size(), 0);
  std::vector<int> loop_end(loop_parent.size(), -1);

  for (int block_id : order) {
    if (block_id == 0) {
      loop_depth[0] = 0;
      continue;
    }

    if (loop_header[block_id]) {
      // loop depth is loop parent's depth + 1
      loop_depth[block_id] = loop_depth[loop_parent[block_id]] + 1;
    } else {
      // loop depth is that of the loop parents depth
      loop_depth[block_id] = loop_depth[loop_parent[block_id]];

      int x = block_id;
      while (true) {
        x = loop_parent[x];
        loop_end[x] = std::max(loop_end[x], block_id);
        if (x == 0) {
          break;
        }
      }
    }
  }

  for (int i = 0; i < loop_parent.size(); i++) {
    if (loop_header[i]) {
      loop_depth[i]--;
    }
  }

  return {loop_depth, loop_end};
}

std::vector<int> ComputeInstrToBB(const std::vector<uint64_t>& instrs,
                                  std::vector<int>& order,
                                  const std::vector<std::pair<int, int>>& bb) {
  std::vector<int> instr_to_bb(instrs.size(), -1);

  for (auto bb_idx : order) {
    const auto& [bb_begin, bb_end] = bb[bb_idx];
    for (int i = bb_begin; i <= bb_end; i++) {
      instr_to_bb[i] = bb_idx;
    }
  }

  return instr_to_bb;
}

LiveIntervalAnalysis ComputeLiveIntervals(const Function& func,
                                          const TypeManager& manager) {
  const auto& bb = func.BasicBlocks();
  const auto& instrs = func.Instructions();

  auto [loop_parent, loop_header] = FindLoops(func);
  auto [labels, order] = LabelBlocks(func, loop_parent);
  auto [loop_depth, loop_end] =
      ComputeLoopDepthEnd(loop_parent, loop_header, order);

  auto instr_to_bb = ComputeInstrToBB(instrs, order, bb);

  // for each value:
  //   for each use of that value:
  //     if value is at same depth as def:
  //        extend live interval to be from def to value's block
  //     else:
  //        walk up loop parent until at depth of def
  //        extend live interval to be from def to end of loop

  std::vector<LiveInterval> live_intervals;
  live_intervals.reserve(instrs.size());
  for (int i = 0; i < instrs.size(); i++) {
    auto v = Value(i);
    live_intervals.emplace_back(v, TypeOf(instrs[i], instrs, manager));
  }

  for (auto bb_idx : order) {
    const auto& [bb_begin, bb_end] = bb[bb_idx];
    for (int i = bb_begin; i <= bb_end; i++) {
      auto instr = instrs[i];
      auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

      if (opcode == Opcode::PHI || opcode == Opcode::GEP_OFFSET ||
          opcode == Opcode::RETURN) {
        continue;
      }

      std::vector<Value> values = ComputeReadValues(instr, instrs);
      for (auto v : values) {
        auto def_bb = instr_to_bb[v.GetIdx()];
        auto use_bb = bb_idx;

        if (loop_depth[def_bb] == loop_depth[use_bb]) {
          live_intervals[v.GetIdx()].Extend(labels[use_bb], i);
        } else {
          if (!(loop_depth[use_bb] > loop_depth[def_bb])) {
            throw std::runtime_error("Invalid def/use situation.");
          }

          if (!loop_header[use_bb]) {
            use_bb = loop_parent[use_bb];
          }

          while (loop_depth[use_bb] > loop_depth[def_bb] + 1) {
            use_bb = loop_parent[use_bb];
          }

          assert(loop_header[use_bb]);
          auto end_of_loop_bb = loop_end[use_bb];
          auto end_of_loop_idx = bb[loop_end[use_bb]].second;
          live_intervals[v.GetIdx()].Extend(labels[end_of_loop_bb],
                                            end_of_loop_idx);
        }
      }

      if (opcode == Opcode::PHI_MEMBER) {
        // phi is read/written here
        Type2InstructionReader reader(instr);
        Value v0(reader.Arg0());
        auto phi = v0.GetIdx();

        live_intervals[phi].Extend(labels[bb_idx], i);
      } else {
        if (DoesWriteValue(instr, manager)) {
          live_intervals[i].Extend(labels[bb_idx], i);
        }

        // extend the live interval to the call for each argument
        if (opcode == Opcode::CALL_ARG) {
          for (int j = i + 1; j <= bb_end; j++) {
            auto opcode =
                OpcodeFrom(GenericInstructionReader(instrs[j]).Opcode());
            if (opcode == Opcode::CALL || opcode == Opcode::CALL_INDIRECT) {
              live_intervals[i].Extend(labels[bb_idx], j);
              break;
            }
          }
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

  /*
  std::cerr << "digraph G {\n";
  for (int i = 0; i < labels.size(); i++) {
    for (auto j : func.BasicBlockSuccessors()[i]) {
      std::cerr << " \"" << i << "\\nlabel:" << labels[i]
                << "\\nloop_parent:" << loop_parent[i]
                << "\\nloop_depth:" << loop_depth[i] << "\" -> \"" << j
                << "\\nlabel:" << labels[j]
                << "\\nloop_parent:" << loop_parent[j]
                << "\\nloop_depth:" << loop_depth[j] << "\"" << std::endl;
    }
  }
  std::cerr << "}" << std::endl;

  for (int i = 0; i < live_intervals.size(); i++) {
    std::cerr << "%" << i << " [" << live_intervals[i].StartBB() << ','
              << live_intervals[i].StartIdx() << "] "
              << " [" << live_intervals[i].EndBB() << ','
              << live_intervals[i].EndIdx() << "]"
              << "\n";
  }
  std::cerr << std::endl;
  */

  LiveIntervalAnalysis result;
  result.live_intervals = std::move(outputs);
  result.order = std::move(order);
  result.labels = std::move(labels);
  return result;
}

}  // namespace kush::khir