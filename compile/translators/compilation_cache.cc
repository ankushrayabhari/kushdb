#include "compile/translators/compilation_cache.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "khir/asm/asm_backend.h"
#include "khir/asm/reg_alloc_impl.h"
#include "khir/backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"

namespace kush::compile {

CacheEntry::CacheEntry() : func_(nullptr), visits_(0) {}

bool CacheEntry::IsCompiled() const { return func_ != nullptr; }

void* CacheEntry::Func() const { return func_; }

int64_t CacheEntry::Visits() { return visits_; }

void CacheEntry::Compile(std::unique_ptr<khir::Program> program,
                         std::string_view main_name,
                         khir::BackendType backend) {
  program_ = std::move(program);

  switch (backend) {
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

RecompilingCache::RecompilingCache(int n) : root_(n), num_tables_(n) {}

RecompilingCache::TrieNode::TrieNode(int n) : children_(n), entry_(nullptr) {}

std::unique_ptr<RecompilingCache::TrieNode>&
RecompilingCache::TrieNode::GetChild(int i) {
  return children_[i];
}

CacheEntry& RecompilingCache::TrieNode::GetEntry() {
  if (entry_ == nullptr) {
    entry_ = std::make_unique<CacheEntry>();
  }
  return *entry_;
}

CacheEntry& RecompilingCache::GetOrInsert(const std::vector<int>& order) {
  assert(order.size() == num_tables_);

  TrieNode* curr = &root_;
  for (int x : order) {
    auto& child = curr->GetChild(x);
    if (child == nullptr) {
      child = std::make_unique<TrieNode>(num_tables_);
    }
    curr = child.get();
  }

  auto& result = curr->GetEntry();
  result.visits_++;
  visits_++;
  return result;
}

int64_t RecompilingCache::Visits() { return visits_; }

PermutableCache::PermutableCache(int num_tables)
    : num_tables_(num_tables),
      table_handlers_(new void*[num_tables]),
      num_flags_(-1),
      flags_(nullptr) {}

int8_t* PermutableCache::AllocateFlags(int num_flags) {
  num_flags_ = num_flags;
  flags_ = new int8_t[num_flags];
  return flags_;
}

void PermutableCache::SetValidTupleHandler(void* valid) {
  valid_tuple_handler_ = valid;
}

void PermutableCache::SetFlagInfo(
    int total_preds,
    absl::flat_hash_map<std::pair<int, int>, int> pred_table_to_flag) {
  total_preds_ = total_preds;
  pred_table_to_flag_ = std::move(pred_table_to_flag);
}

void* PermutableCache::Order(const std::vector<int>& order) {
  // Reset all flags to 0
  for (int i = 0; i < num_flags_; i++) {
    flags_[i] = 0;
  }

  // Set all predicate flags
  absl::flat_hash_set<int> available_tables;
  absl::flat_hash_set<int> executed_preds;
  for (int order_idx = 0; order_idx < order.size(); order_idx++) {
    auto table = order[order_idx];
    available_tables.insert(table);

    // all tables in available_tables have been joined
    // execute any non-executed general predicate
    for (int pred = 0; pred < total_preds_; pred++) {
      if (executed_preds.contains(pred)) {
        continue;
      }

      bool all_tables_available = true;
      for (int t = 0; t < order.size(); t++) {
        if (pred_table_to_flag_.contains({pred, t})) {
          all_tables_available =
              all_tables_available && available_tables.contains(t);
        }
      }
      if (!all_tables_available) {
        continue;
      }
      executed_preds.insert(pred);

      auto flag = pred_table_to_flag_.at({pred, table});
      flags_[flag] = 1;
    }
  }

  for (int i = 0; i < order.size() - 1; i++) {
    // set the ith handler to i+1 function
    int current_table = order[i];
    int next_table = order[i + 1];

    table_handlers_[current_table] = table_functions_[next_table];
  }

  // set last handler to be the valid tuple handler
  table_handlers_[order[order.size() - 1]] = valid_tuple_handler_;

  return table_functions_[order[0]];
}

void* PermutableCache::TableHandlers() { return table_handlers_; }

bool PermutableCache::IsCompiled() const { return program_ != nullptr; }

void PermutableCache::Compile(std::unique_ptr<khir::Program> program,
                              std::vector<std::string> function_names,
                              khir::BackendType backend_type) {
  program_ = std::move(program);

  switch (backend_type) {
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

  table_functions_ = std::vector<void*>(num_tables_);
  for (int i = 0; i < num_tables_; i++) {
    table_functions_[i] = backend_->GetFunction(function_names[i]);
  }
}

}  // namespace kush::compile