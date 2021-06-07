#include "khir/opcode.h"

#include <cstdint>

namespace kush::khir {

ConstantOpcode ConstantOpcodeFrom(uint8_t t) {
  return static_cast<ConstantOpcode>(t);
}

uint8_t ConstantOpcodeTo(ConstantOpcode opcode) {
  return static_cast<uint8_t>(opcode);
}

Opcode OpcodeFrom(uint8_t t) { return static_cast<Opcode>(t); }

uint8_t OpcodeTo(Opcode opcode) { return static_cast<uint8_t>(opcode); }

}  // namespace kush::khir
