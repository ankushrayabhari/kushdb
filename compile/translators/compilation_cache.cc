#include "compile/translators/compilation_cache.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "khir/asm/asm_backend.h"
#include "khir/asm/reg_alloc_impl.h"
#include "khir/backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"

namespace kush::compile {

CacheEntry::CacheEntry() : func_(nullptr) {}

bool CacheEntry::IsCompiled() const { return func_ != nullptr; }

void* CacheEntry::Func() const { return func_; }

void CacheEntry::Compile(std::unique_ptr<khir::Program> program,
                         std::string_view main_name) {
  program_ = std::move(program);

  switch (khir::GetBackendType()) {
    case khir::BackendType::ASM: {
      auto backend =
          std::make_unique<khir::ASMBackend>(*program_, khir::RegAllocImpl());
      backend->Compile();
      backend_ = std::move(backend);
      break;
    }

    case khir::BackendType::LLVM: {
      auto backend = std::make_unique<khir::LLVMBackend>(*program_);
      backend_ = std::move(backend);
      break;
    }
  }

  func_ = backend_->GetFunction(main_name);
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

}  // namespace kush::compile