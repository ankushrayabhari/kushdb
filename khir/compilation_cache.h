#pragma once

#include <memory>
#include <vector>

#include "khir/program_builder.h"

namespace kush::khir {

class CacheEntry {
 public:
  CacheEntry();
  bool IsCompiled() const;
  void* Func(std::string_view name) const;
  khir::ProgramBuilder& ProgramBuilder();
  void Compile();

 private:
  khir::ProgramBuilder program_builder_;
  std::unique_ptr<Backend> compiled_program_;
};

class CompilationCache {
 public:
  CompilationCache(int n);
  CacheEntry& GetOrInsert(const std::vector<int>& order);

 private:
  class TrieNode {
   public:
    TrieNode(int n);
    std::unique_ptr<TrieNode>& GetChild(int i);
    CacheEntry& GetEntry();

   private:
    std::vector<std::unique_ptr<TrieNode>> children_;
    std::unique_ptr<CacheEntry> entry_;
  };

  TrieNode root_;
  int num_tables_;
};

}  // namespace kush::khir