
Select::Select(OperatorSchema schema, std::unique_ptr<Operator> child,
               std::unique_ptr<Expression> e)
    : UnaryOperator(std::move(schema), {std::move(child)}),
      expression(std::move(e)) {
  children_[0]->SetParent(this);
}

nlohmann::json Select::ToJson() const {
  nlohmann::json j;
  j["op"] = "SELECT";
  j["relation"] = children_[0]->ToJson();
  j["expression"] = expression->ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void Select::Accept(OperatorVisitor& visitor) { visitor.Visit(*this); }