#pragma once

#include "compile/llvm/llvm_program.h"

#define INSTANTIATE_ON_BACKENDS(T) template class T<kush::compile::LLVMImpl>;