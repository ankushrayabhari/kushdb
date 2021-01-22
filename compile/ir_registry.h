#pragma once

#include "compile/llvm/llvm_ir.h"

#define INSTANTIATE_ON_IR(T) template class T<kush::compile::ir::LLVMIrTypes>;