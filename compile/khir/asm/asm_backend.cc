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

ASMBackend::ASMBackend() {
  code_.init(rt_.environment());
  asm_ = std::make_unique<asmjit::x86::Assembler>(&code_);
  text_section_ = code_.textSection();
  code_.newSection(&data_section_, ".data", SIZE_MAX, 0, 8, 0);
}

void ASMBackend::TranslateGlobalConstCharArray(std::string_view s) {}

}  // namespace kush::khir