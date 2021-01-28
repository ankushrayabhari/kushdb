#pragma once

#include <optional>

namespace kush::util {

template <typename VisitorImplementation, typename Target, typename Data>
class Visitor : public VisitorImplementation {
 public:
  Visitor() = default;
  virtual ~Visitor() = default;

  std::unique_ptr<Data> Compute(Target target) {
    target.Accept(*this);
    assert(result_ != nullptr);
    auto value = std::move(result_);
    result_.reset();
    return std::move(value);
  }

 protected:
  void Return(std::unique_ptr<Data> result) { result_ = std::move(result); }

 private:
  std::unique_ptr<Data> result_;
};

}  // namespace kush::util