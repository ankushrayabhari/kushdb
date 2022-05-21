#pragma once

#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace kush::execution {

class QueryState {
 public:
  QueryState() = default;
  QueryState(const QueryState&) = delete;
  QueryState(QueryState&& st) : values_(std::move(st.values_)) {}
  virtual ~QueryState() {
    for (auto v : values_) {
      free(v);
    }
  }
  QueryState& operator=(const QueryState&) = delete;
  QueryState& operator=(QueryState&& st) {
    values_ = std::move(st.values_);
    return *this;
  }

  template <typename T>
  void* Allocate() {
    auto dest = malloc(sizeof(T));
    values_.push_back(dest);
    return dest;
  }

  void* Allocate(uint64_t size) {
    auto dest = malloc(size);
    values_.push_back(dest);
    return dest;
  }

 private:
  std::vector<void*> values_;
};

}  // namespace kush::execution