#pragma once

#include <optional>

namespace kush::util {

template <typename Target, typename VisitorImplementation, typename Data>
class Visitor : public VisitorImplementation {
 public:
  Visitor() = default;
  virtual ~Visitor() = default;

  Data Compute(Target target) {
    target.Accept(*this);
    assert(result_.has_value());
    auto value = std::move(result_.value());
    result_.reset();
    return std::move(value);
  }

 protected:
  void Return(Data result) { result_ = std::move(result); }

 private:
  std::optional<Data> result_;
};

}  // namespace kush::util