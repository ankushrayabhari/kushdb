#pragma once

#include <optional>

namespace kush::util {

template <typename VisitorImplementation, typename Target, typename Data>
class Visitor : public VisitorImplementation {
 public:
  Visitor() = default;
  virtual ~Visitor() = default;

  Data Compute(Target target) {
    target.Accept(*this);
    if (!result_.has_value()) {
      throw std::runtime_error("No value was returned.");
    }
    auto v = std::move(result_.value());
    result_ = std::nullopt;
    return v;
  }

 protected:
  void Return(Data result) { result_ = std::move(result); }

 private:
  std::optional<Data> result_;
};

}  // namespace kush::util