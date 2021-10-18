#include <filesystem>
#include <iostream>
#include <string>

#include "hyperapi/hyperapi.hpp"

static const hyperapi::TableDefinition region{
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

static void Region(hyperapi::HyperProcess& hyper, std::string dest_path,
                   std::string raw) {
  {
    hyperapi::Connection connection(hyper.getEndpoint(), dest_path,
                                    hyperapi::CreateMode::CreateAndReplace);
    const hyperapi::Catalog& catalog = connection.getCatalog();
    catalog.createTable(region);

    // Using path to current file, create a path that locates CSV file
    // packaged with these examples.
    std::string csv_path = raw + "region.tbl";

    int64_t rowCount = connection.executeCommand(
        "COPY " + region.getTableName().toString() + " FROM " +
        hyperapi::escapeStringLiteral(csv_path) +
        " WITH (FORMAT CSV, DELIMITER '|')");

    std::cout << "Finished " << region.getTableName() << ": " << rowCount
              << std::endl;
  }
}

int main(int argc, const char** argv) {
  auto dest_path = "benchmark/tpch/hyper/tpch1.hyper";
  auto raw_path = "benchmark/tpch/raw-1/";

  try {
    hyperapi::HyperProcess hyper(
        "bazel-bin/benchmark/tpch/hyper_load.runfiles/hyper/lib/hyper/",
        hyperapi::Telemetry::DoNotSendUsageDataToTableau);
    Region(hyper, dest_path, raw_path);
  } catch (const hyperapi::HyperException& e) {
    std::cerr << e.toString() << std::endl;
    return 1;
  }
  return 0;
}