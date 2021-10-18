#include <filesystem>
#include <iostream>
#include <string>

#include "hyperapi/hyperapi.hpp"

const hyperapi::TableDefinition region{
    "region",
    {
        hyperapi::TableDefinition::Column{"r_regionkey",
                                          hyperapi::SqlType::integer(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"r_name", hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"r_comment",
                                          hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
    }};

void Region(hyperapi::HyperProcess& hyper, std::string dest_path,
            std::string raw) {
  {
    hyperapi::Connection connection(hyper.getEndpoint(), dest_path,
                                    hyperapi::CreateMode::CreateAndReplace);
    const hyperapi::Catalog& catalog = connection.getCatalog();
    catalog.createTable(region);

    // Using path to current file, create a path that locates CSV file
    // packaged with these examples.
    std::string csv_path = raw + "region.tbl.tight";

    int64_t rowCount = connection.executeCommand(
        "COPY " + region.getTableName().toString() + " FROM " +
        hyperapi::escapeStringLiteral(csv_path) +
        " WITH (FORMAT CSV, DELIMITER '|')");

    std::cout << "Finished " << region.getTableName() << ": " << rowCount
              << std::endl;
  }
}

const hyperapi::TableDefinition nation{
    "nation",
    {
        hyperapi::TableDefinition::Column{"n_nationkey",
                                          hyperapi::SqlType::integer(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"n_name", hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"n_regionkey",
                                          hyperapi::SqlType::integer(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"n_comment",
                                          hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
    }};

void Nation(hyperapi::HyperProcess& hyper, std::string dest_path,
            std::string raw) {
  {
    hyperapi::Connection connection(hyper.getEndpoint(), dest_path,
                                    hyperapi::CreateMode::CreateAndReplace);
    const hyperapi::Catalog& catalog = connection.getCatalog();
    catalog.createTable(nation);

    // Using path to current file, create a path that locates CSV file
    // packaged with these examples.
    std::string csv_path = raw + "nation.tbl.tight";

    int64_t rowCount = connection.executeCommand(
        "COPY " + nation.getTableName().toString() + " FROM " +
        hyperapi::escapeStringLiteral(csv_path) +
        " WITH (FORMAT CSV, DELIMITER '|')");

    std::cout << "Finished " << nation.getTableName() << ": " << rowCount
              << std::endl;
  }
}

const hyperapi::TableDefinition part{
    "part",
    {
        hyperapi::TableDefinition::Column{"p_partkey",
                                          hyperapi::SqlType::integer(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_name", hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_mfgr", hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_brand", hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_type", hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_size",
                                          hyperapi::SqlType::integer(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_container",
                                          hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_retailprice",
                                          hyperapi::SqlType::doublePrecision(),
                                          hyperapi::Nullability::NotNullable},
        hyperapi::TableDefinition::Column{"p_comment",
                                          hyperapi::SqlType::text(),
                                          hyperapi::Nullability::NotNullable},
    }};

void Part(hyperapi::HyperProcess& hyper, std::string dest_path,
          std::string raw) {
  {
    hyperapi::Connection connection(hyper.getEndpoint(), dest_path,
                                    hyperapi::CreateMode::CreateAndReplace);
    const hyperapi::Catalog& catalog = connection.getCatalog();
    catalog.createTable(part);

    // Using path to current file, create a path that locates CSV file
    // packaged with these examples.
    std::string csv_path = raw + "part.tbl.tight";

    int64_t rowCount = connection.executeCommand(
        "COPY " + part.getTableName().toString() + " FROM " +
        hyperapi::escapeStringLiteral(csv_path) +
        " WITH (FORMAT CSV, DELIMITER '|')");

    std::cout << "Finished " << part.getTableName() << ": " << rowCount
              << std::endl;
  }
}

int main(int argc, const char** argv) {
  auto hyper_path =
      "bazel-bin/benchmark/tpch/hyper_load.runfiles/hyper/lib/hyper/";

  std::vector<std::pair<std::string, std::string>> databases{
      {"benchmark/tpch/hyper/tpch1.hyper", "benchmark/tpch/raw-1/"},
      {"benchmark/tpch/hyper/tpch10.hyper", "benchmark/tpch/raw-10/"},
  };

  for (const auto& [dest_path, raw_path] : databases) {
    std::cout << "Working on " << raw_path << std::endl;
    try {
      hyperapi::HyperProcess hyper(
          hyper_path, hyperapi::Telemetry::DoNotSendUsageDataToTableau);
      Region(hyper, dest_path, raw_path);
      Nation(hyper, dest_path, raw_path);
      Part(hyper, dest_path, raw_path);
      // Supplier(hyper, dest_path, raw_path);
      // Partsupp(hyper, dest_path, raw_path);
      // Customer(hyper, dest_path, raw_path);
      // Orders(hyper, dest_path, raw_path);
      // Lineitem(hyper, dest_path, raw_path);
    } catch (const hyperapi::HyperException& e) {
      std::cerr << e.toString() << std::endl;
      return 1;
    }
  }
  return 0;
}