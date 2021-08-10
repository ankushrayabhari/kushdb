#pragma once

namespace kush::compile {

enum class Backend { ASM, LLVM };

Backend GetBackend();

}