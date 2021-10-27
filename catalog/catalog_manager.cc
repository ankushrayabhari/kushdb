#include "catalog/catalog_manager.h"

#include "catalog/catalog.h"

namespace kush::catalog {

void CatalogManager::SetCurrent(Database db) { db_ = std::move(db); }

const Database& CatalogManager::Current() const { return db_; }

}  // namespace kush::catalog
