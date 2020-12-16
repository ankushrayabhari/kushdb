

class HashJoin final : public BinaryOperator {
 public:
  HashJoin(OperatorSchema schema, std::unique_ptr<Operator> left,
           std::unique_ptr<Operator> right,
           std::unique_ptr<ColumnRefExpression> left_column_,
           std::unique_ptr<ColumnRefExpression> right_column_);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
  ColumnRefExpression& LeftColumn();
  ColumnRefExpression& RightColumn();

 private:
  std::unique_ptr<ColumnRefExpression> left_column_;
  std::unique_ptr<ColumnRefExpression> right_column_;
};