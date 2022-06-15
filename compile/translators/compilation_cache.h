#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "khir/backend.h"
#include "khir/program_builder.h"

namespace kush::compile {

class CompilationCache;

class CacheEntry {
 public:
  CacheEntry();
  bool IsCompiled() const;
  void* Func() const;
  void Compile(std::unique_ptr<khir::Program> program,
               std::string_view main_name, khir::BackendType backend);
  int64_t Visits();

 private:
  friend class CompilationCache;
  std::unique_ptr<khir::Program> program_;
  std::unique_ptr<khir::Backend> backend_;
  void* func_;
  int64_t visits_;
};

class CompilationCache {
 public:
  CompilationCache(int n);
  CacheEntry& GetOrInsert(const std::vector<int>& order);
  int64_t Visits();

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
  int64_t visits_;
};

}  // namespace kush::compile