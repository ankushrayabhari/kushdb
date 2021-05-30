#include "compile/khir/asm/asm_backend.h"

#include <stdexcept>

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "compile/khir/instruction.h"
#include "compile/khir/program_builder.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

void ExceptionErrorHandler::handleError(asmjit::Error err, const char* message,
                                        asmjit::BaseEmitter* origin) {
  throw std::runtime_error(message);
}

void ASMBackend::Init(const TypeManager& manager,
                      const std::vector<uint64_t>& i64_constants,
                      const std::vector<double>& f64_constants,
                      const std::vector<std::string>& char_array_constants,
                      const std::vector<StructConstant>& struct_constants,
                      const std::vector<ArrayConstant>& array_constants) {
  code_.init(rt_.environment());
  asm_ = std::make_unique<asmjit::x86::Assembler>(&code_);
  text_section_ = code_.textSection();
  code_.newSection(&data_section_, ".data", SIZE_MAX, 0, 8, 0);
}

void ASMBackend::Translate(const std::vector<Global>& globals,
                           const std::vector<Function>& functions) {
  // TODO
}

void ASMBackend::Compile() const {
  // TODO
}

void ASMBackend::Execute() const {
  // TODO
}

}  // namespace kush::khir