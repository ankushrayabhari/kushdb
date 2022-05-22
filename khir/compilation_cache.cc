#include "khir/compilation_cache.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "khir/asm/asm_backend.h"
#include "khir/asm/reg_alloc_impl.h"
#include "khir/backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"

namespace kush::khir {

CacheEntry::CacheEntry() : program_(nullptr) {}

bool CacheEntry::IsCompiled() const { return program_ != nullptr; }

void* CacheEntry::Func(std::string_view name) const {
  return backend_->GetFunction(name);
}

void CacheEntry::Compile(std::unique_ptr<Program> program) {
  program_ = std::move(program);

  switch (GetBackendType()) {
    case BackendType::ASM: {
      auto backend =
          std::make_unique<ASMBackend>(*program_, khir::RegAllocImpl());
      backend->Compile();
      backend_ = std::move(backend);
      break;
    }

    case BackendType::LLVM: {
      auto backend = std::make_unique<LLVMBackend>(*program_);
      backend_ = std::move(backend);
      break;
    }
  }
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