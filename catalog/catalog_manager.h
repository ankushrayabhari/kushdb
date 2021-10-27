#pragma once

#include <string>
#include <string_view>

#include "catalog/catalog.h"

namespace kush::catalog {

class CatalogManager {
 public:
  static CatalogManager& Get() {
    static CatalogManager instance;
    return instance;
  }

 private:
  CatalogManager() = default;
  ~CatalogManager();

 public:
  CatalogManager(CatalogManager const&) = delete;
  void operator=(CatalogManager const&) = delete;

  void SetCurrent(Database db);
  const Database& Current() const;

 private:
  Database db_;
};

}  // namespace kush::catalog
