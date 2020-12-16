
class Select final : public UnaryOperator {
 public:
  std::unique_ptr<Expression> expression;
  Select(OperatorSchema schema, std::unique_ptr<Operator> child,
         std::unique_ptr<Expression> e);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};