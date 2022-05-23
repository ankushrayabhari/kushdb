#pragma once

#include <memory>
#include <vector>

#include "khir/backend.h"
#include "khir/program_builder.h"

namespace kush::compile {

class CacheEntry {
 public:
  CacheEntry();
  bool IsCompiled() const;
  void* Func() const;
  void Compile(std::unique_ptr<khir::Program> program,
               std::string_view main_name);

 private:
  std::unique_ptr<khir::Program> program_;
  std::unique_ptr<khir::Backend> backend_;
  void* func_;
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

}  // namespace kush::compile