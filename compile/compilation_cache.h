#pragma once

#include <memory>
#include <vector>

#include "compile/program.h"
#include "khir/program_builder.h"

namespace kush::compile {

class CacheEntry {
 public:
  CacheEntry();
  bool IsCompiled() const;
  void* Func(std::string_view name) const;
  khir::ProgramBuilder& ProgramBuilder();
  void Compile();

 private:
  khir::ProgramBuilder program_builder_;
  std::unique_ptr<compile::Program> compiled_program_;
};

class CompilationCache {
 public:
  CompilationCache(int n);
  CacheEntry& GetOrInsert(const std::vector<int>& order);

 private:
  struct TrieNode {
    TrieNode(int n);
    CacheEntry& GetEntry();
    std::vector<std::unique_ptr<TrieNode>> children_;
    std::unique_ptr<CacheEntry> entry_;
  };

  TrieNode root_;
  int num_tables_;
};

}  // namespace kush::compile