#pragma once

#include "llvm/ExecutionEngine/JITEventListener.h"

// Very hacky way of getting the PerfJITEventListener
// Currently, the LLVM Bazel files are very early stage:
// - PerfJitEventListener.cpp isn't included in a cc_library
// - Configuration preprocessor macros like LLVM_USE_PERF can't be set
//
// Instead, we include the source file for PerfJitEventListener and explicitly,
// write a method to access the JITEventListener.

namespace kush::khir {

llvm::JITEventListener *createPerfJITEventListener();

}
