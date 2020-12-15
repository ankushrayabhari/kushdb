#pragma once

namespace kush::util {

template <typename VisitorImplementation, typename Data>
class Visitor : public VisitorImplementation {
 public:
  Visitor();

 protected:
  void Return(Data result) { result_ = std::move(result); }
  Data GetResult() { return std::move(result_); }

 private:
  Data result_;
};

}  // namespace kush::util