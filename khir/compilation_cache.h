#pragma once

#include <memory>
#include <vector>

#include "khir/backend.h"
#include "khir/program_builder.h"

namespace kush::khir {

class CacheEntry {
 public:
  CacheEntry();
  bool IsCompiled() const;
  void* Func(std::string_view name) const;
  void Compile(std::unique_ptr<Program> program);

 private:
  std::unique_ptr<Program> program_;
  std::unique_ptr<Backend> backend_;
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