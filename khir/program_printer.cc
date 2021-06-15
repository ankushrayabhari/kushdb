#include "khir/program_printer.h"

#include <iostream>

#include "khir/instruction.h"
#include "khir/opcode.h"
#include "khir/program_builder.h"
#include "magic_enum.hpp"

namespace kush::khir {

void OutputValue(khir::Value v, const std::vector<uint64_t>& instrs,
                 const std::vector<uint64_t>& constant_instrs,
                 const std::vector<uint64_t>& i64_constants,
                 const std::vector<double>& f64_constants,
                 const std::vector<std::string>& char_array_constants,
                 const std::vector<StructConstant>& struct_constants,
                 const std::vector<ArrayConstant>& array_constants,
                 const std::vector<Global>& globals,
                 const std::vector<Function>& functions) {
  if (v.IsConstantGlobal()) {
    auto instr = constant_instrs[v.GetIdx()];
    auto opcode = ConstantOpcodeFrom(GenericInstructionReader(instr).Opcode());

    switch (opcode) {
      case ConstantOpcode::I1_CONST: {
        int32_t v = Type1InstructionReader(instr).Constant();
        std::cerr << "i1:" << v;
        return;
      }

      case ConstantOpcode::I8_CONST: {
        int32_t v = Type1InstructionReader(instr).Constant();
        std::cerr << "i8:" << v;
        return;
      }

      case ConstantOpcode::I16_CONST: {
        int32_t v = Type1InstructionReader(instr).Constant();
        std::cerr << "i16:" << v;
        return;
      }

      case ConstantOpcode::I32_CONST: {
        int32_t v = Type1InstructionReader(instr).Constant();
        std::cerr << "i32:" << v;
        return;
      }

      case ConstantOpcode::I64_CONST: {
        int v = Type1InstructionReader(instr).Constant();
        std::cerr << "i64:" << i64_constants[v];
        return;
      }

      case ConstantOpcode::F64_CONST: {
        int v = Type1InstructionReader(instr).Constant();
        std::cerr << "f64:" << f64_constants[v];
        return;
      }

      case ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST: {
        int v = Type1InstructionReader(instr).Constant();
        std::cerr << "$" << v;
        return;
      }

      case ConstantOpcode::STRUCT_CONST:
      case ConstantOpcode::ARRAY_CONST: {
        return;
      }

      case ConstantOpcode::NULLPTR: {
        std::cerr << "nullptr";
        return;
      }

      case ConstantOpcode::GLOBAL_REF: {
        int v = Type1InstructionReader(instr).Constant();
        std::cerr << "#" << v;
        return;
      }

      case ConstantOpcode::FUNC_PTR: {
        std::cerr << functions[Type3InstructionReader(instr).Arg()].Name();
        return;
      }
    }
  }

  auto instr = instrs[v.GetIdx()];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());
  if (opcode == Opcode::FUNC_ARG) {
    int idx = Type3InstructionReader(instr).Sarg();
    std::cerr << "%a" << idx;
    return;
  }

  std::cerr << "%" << v.GetIdx();
}

void ProgramPrinter::OutputInstr(
    int idx, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<std::string>& char_array_constants,
    const std::vector<StructConstant>& struct_constants,
    const std::vector<ArrayConstant>& array_constants,
    const std::vector<Global>& globals,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<Function>& functions, const Function& func) {
  auto instrs = func.Instructions();

  auto opcode = OpcodeFrom(GenericInstructionReader(instrs[idx]).Opcode());
  switch (opcode) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_DIV:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE:
    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_DIV:
    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_DIV:
    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I64_DIV:
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
    case Opcode::PHI_MEMBER:
    case Opcode::GEP_OFFSET: {
      Type2InstructionReader reader(instrs[idx]);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      std::cerr << "   %" << idx << " = " << magic_enum::enum_name(opcode)
                << " ";
      OutputValue(v0, instrs, constant_instrs, i64_constants, f64_constants,
                  char_array_constants, struct_constants, array_constants,
                  globals, functions);
      std::cerr << " ";
      OutputValue(v1, instrs, constant_instrs, i64_constants, f64_constants,
                  char_array_constants, struct_constants, array_constants,
                  globals, functions);
      std::cerr << "\n";

      return;
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
    case Opcode::F64_CONV_I64:
    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instrs[idx]);
      Value v0(reader.Arg0());

      std::cerr << "   %" << idx << " = " << magic_enum::enum_name(opcode)
                << " ";
      OutputValue(v0, instrs, constant_instrs, i64_constants, f64_constants,
                  char_array_constants, struct_constants, array_constants,
                  globals, functions);
      std::cerr << "\n";

      return;
    }

    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instrs[idx]);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      std::cerr << "   " << magic_enum::enum_name(opcode) << " ";
      OutputValue(v0, instrs, constant_instrs, i64_constants, f64_constants,
                  char_array_constants, struct_constants, array_constants,
                  globals, functions);
      std::cerr << " ";
      OutputValue(v1, instrs, constant_instrs, i64_constants, f64_constants,
                  char_array_constants, struct_constants, array_constants,
                  globals, functions);
      std::cerr << "\n";
      return;
    }

    case Opcode::BR: {
      Type5InstructionReader reader(instrs[idx]);
      auto label = reader.Marg0();
      std::cerr << "   " << magic_enum::enum_name(opcode) << " ." << label
                << "\n";
      return;
    }

    case Opcode::RETURN: {
      std::cerr << "   " << magic_enum::enum_name(opcode) << "\n";
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instrs[idx]);
      Value v0(reader.Arg());

      std::cerr << "   " << magic_enum::enum_name(opcode) << " ";
      OutputValue(v0, instrs, constant_instrs, i64_constants, f64_constants,
                  char_array_constants, struct_constants, array_constants,
                  globals, functions);
      int label0 = reader.Marg0();
      int label1 = reader.Marg1();
      std::cerr << " ." << label0 << " ." << label1 << "\n";
      return;
    }

    case Opcode::FUNC_ARG: {
      return;
    }

    case Opcode::PTR_CAST:
    case Opcode::PTR_LOAD:
    case Opcode::RETURN_VALUE:
    case Opcode::CALL_ARG:
    case Opcode::GEP:
    case Opcode::CALL_INDIRECT: {
      Type3InstructionReader reader(instrs[idx]);
      Value v0(reader.Arg());

      std::cerr << "   %" << idx << " = " << magic_enum::enum_name(opcode)
                << " ";
      OutputValue(v0, instrs, constant_instrs, i64_constants, f64_constants,
                  char_array_constants, struct_constants, array_constants,
                  globals, functions);
      std::cerr << "\n";

      return;
    }

    case Opcode::PHI:
    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instrs[idx]);
      Value v0(reader.Arg());

      std::cerr << "   %" << idx << " = " << magic_enum::enum_name(opcode)
                << "\n";
      return;
    }

    case Opcode::CALL: {
      Type3InstructionReader reader(instrs[idx]);
      Value v0(reader.Arg());

      if (!manager_->IsVoid(static_cast<Type>(reader.TypeID()))) {
        std::cerr << "   %" << idx << " = ";
      } else {
        std::cerr << "   ";
      }

      std::cerr << magic_enum::enum_name(opcode) << " "
                << functions[reader.Arg()].Name() << "\n";
      return;
    }
  }
}

void ProgramPrinter::TranslateVoidType() { type_to_string_.push_back("void"); }

void ProgramPrinter::TranslateI1Type() { type_to_string_.push_back("i1"); }

void ProgramPrinter::TranslateI8Type() { type_to_string_.push_back("i8"); }

void ProgramPrinter::TranslateI16Type() { type_to_string_.push_back("i16"); }

void ProgramPrinter::TranslateI32Type() { type_to_string_.push_back("i32"); }

void ProgramPrinter::TranslateI64Type() { type_to_string_.push_back("i64"); }

void ProgramPrinter::TranslateF64Type() { type_to_string_.push_back("f64"); }

void ProgramPrinter::TranslatePointerType(Type elem) {
  if (manager_->IsI1Type(elem) || manager_->IsI8Type(elem) ||
      manager_->IsI16Type(elem) || manager_->IsI64Type(elem) ||
      manager_->IsI32Type(elem) || manager_->IsF64Type(elem) ||
      manager_->IsPtrType(elem)) {
    type_to_string_.push_back(type_to_string_[elem.GetID()] + "*");
  } else {
    type_to_string_.push_back("(" + type_to_string_[elem.GetID()] + ")*");
  }
}

void ProgramPrinter::TranslateArrayType(Type elem, int len) {
  type_to_string_.push_back(type_to_string_[elem.GetID()] + "[" +
                            (len > 0 ? std::to_string(len) : "") + "]");
}
void ProgramPrinter::TranslateFunctionType(Type result,
                                           absl::Span<const Type> arg_types) {
  std::string output = "(";
  for (int i = 0; i < arg_types.size(); i++) {
    output.append(type_to_string_[arg_types[i].GetID()]);
    if (i < arg_types.size() - 1) {
      output.append(", ");
    }
  }
  output.append(") -> ");
  output.append(type_to_string_[result.GetID()]);
  type_to_string_.push_back(output);
}

void ProgramPrinter::TranslateStructType(absl::Span<const Type> elem_types) {
  std::string output = "{";
  for (int i = 0; i < elem_types.size(); i++) {
    output.append("%" + std::to_string(i) + ": " +
                  type_to_string_[elem_types[i].GetID()]);
    if (i < elem_types.size() - 1) {
      output.append(", ");
    }
  }
  output.append("}");
  type_to_string_.push_back(output);
}

void ProgramPrinter::Translate(
    const TypeManager& manager, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<std::string>& char_array_constants,
    const std::vector<StructConstant>& struct_constants,
    const std::vector<ArrayConstant>& array_constants,
    const std::vector<Global>& globals,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<Function>& functions) {
  manager_ = &manager;
  manager.Translate(*this);

  for (int i = 0; i < char_array_constants.size(); i++) {
    std::cerr << "$" << i << " = \"" << char_array_constants[i] << "\";\n\n";
  }

  for (int i = 0; i < globals.size(); i++) {
    const auto& glob = globals[i];
    std::cerr << "#" << i << " = " << type_to_string_[glob.Type().GetID()]
              << ";\n\n";
  }

  for (const auto& func : functions) {
    const auto& basic_blocks = func.BasicBlocks();

    if (func.External()) {
      std::cerr << "extern " << func.Name()
                << type_to_string_[func.Type().GetID()] << ";\n\n";
      continue;
    }

    std::cerr << func.Name() << type_to_string_[func.Type().GetID()] << " {\n";
    for (int i = 0; i < basic_blocks.size(); i++) {
      std::cerr << " ." << i << ":\n";
      const auto& [i_start, i_end] = basic_blocks[i];
      for (int j = i_start; j <= i_end; j++) {
        OutputInstr(j, i64_constants, f64_constants, char_array_constants,
                    struct_constants, array_constants, globals, constant_instrs,
                    functions, func);
      }
    }
    std::cerr << "}\n" << std::endl;
  }
}

}  // namespace kush::khir