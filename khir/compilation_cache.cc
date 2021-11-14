#include "khir/compilation_cache.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "khir/asm/asm_backend.h"
#include "khir/asm/reg_alloc_impl.h"
#include "khir/backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program.h"
#include "khir/program_builder.h"

namespace kush::khir {

CacheEntry::CacheEntry() : compiled_program_(nullptr) {}

bool CacheEntry::IsCompiled() const { return compiled_program_ != nullptr; }

void* CacheEntry::Func(std::string_view name) const {
  return compiled_program_->GetFunction(name);
}

khir::ProgramBuilder& CacheEntry::ProgramBuilder() { return program_builder_; }

void CacheEntry::Compile() {
  switch (GetBackendType()) {
    case BackendType::ASM: {
      auto backend =
          std::make_unique<khir::ASMBackend>(khir::RegAllocImpl::STACK_SPILL);
      program_builder_.Translate(*backend);
      compiled_program_ = std::move(backend);
      break;
    }

    case BackendType::LLVM: {
      auto backend = std::make_unique<khir::LLVMBackend>();
      program_builder_.Translate(*backend);
      compiled_program_ = std::move(backend);
      break;
    }
  }

  compiled_program_->Compile();
}

CompilationCache::CompilationCache(int n) : root_(n), num_tables_(n) {}

CompilationCache::TrieNode::TrieNode(int n) : children_(n), entry_(nullptr) {}

std::unique_ptr<CompilationCache::TrieNode>&
CompilationCache::TrieNode::GetChild(int i) {
  return children_[i];
}

CacheEntry& CompilationCache::TrieNode::GetEntry() {
  if (entry_ == nullptr) {
    entry_ = std::make_unique<CacheEntry>();
  }
  return *entry_;
}

CacheEntry& CompilationCache::GetOrInsert(const std::vector<int>& order) {
  assert(order.size() == num_tables_);

  TrieNode* curr = &root_;
  for (int x : order) {
    auto& child = curr->GetChild(x);
    if (child == nullptr) {
      child = std::make_unique<TrieNode>(num_tables_);
    }
    curr = child.get();
  }

  return curr->GetEntry();
}

}  // namespace kush::khir