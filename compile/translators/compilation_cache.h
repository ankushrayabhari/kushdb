#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "khir/backend.h"
#include "khir/program_builder.h"

namespace kush::compile {

class RecompilingCache;

class CacheEntry {
 public:
  CacheEntry();
  bool IsCompiled() const;
  void* Func() const;
  void Compile(std::unique_ptr<khir::Program> program,
               std::string_view main_name, khir::BackendType backend);
  int64_t Visits();

 private:
  friend class RecompilingCache;
  std::unique_ptr<khir::Program> program_;
  std::unique_ptr<khir::Backend> backend_;
  void* func_;
  int64_t visits_;
};

class RecompilingCache {
 public:
  RecompilingCache(int n);
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

class PermutableCache {
 public:
  PermutableCache(int num_tables);

  int8_t* AllocateFlags(int num_flags);
  void* TableHandlers();

  bool IsCompiled() const;
  void Compile(std::unique_ptr<khir::Program> program,
               std::vector<std::string> function_names, khir::BackendType type);

  void SetFlagInfo(
      int total_preds,
      absl::flat_hash_map<std::pair<int, int>, int> pred_table_to_flag);

  void* Order(const std::vector<int>& order);

  void SetValidTupleHandler(void* table_handler);

 private:
  int num_tables_;
  int total_preds_;
  void** table_handlers_;
  int num_flags_;
  int8_t* flags_;

  void* valid_tuple_handler_;

  std::vector<void*> table_functions_;
  absl::flat_hash_map<std::pair<int, int>, int> pred_table_to_flag_;

  std::unique_ptr<khir::Program> program_;
  std::unique_ptr<khir::Backend> backend_;
};

}  // namespace kush::compile