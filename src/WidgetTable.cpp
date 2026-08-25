/* -*- C++ -*- */
//
// WidgetTable.cpp
//
// See WidgetTable.h.

#include "WidgetTable.h"

namespace ModernCommon {

std::string
WidgetTable::buildInsertSql(const WidgetEntry& entry)
{
  std::ostringstream sql;
  sql << "INSERT INTO widget (id, name, category, quantity) VALUES ("
      << entry.getId() << ", "
      << DataFactory::getNullableStringColumnValue(entry.getName()) << ", "
      << DataFactory::getNullableStringColumnValue(entry.getCategory()) << ", "
      << entry.getQuantity() << ")";
  return sql.str();
}

std::string
WidgetTable::buildUpdateSql(const WidgetEntry& entry)
{
  std::ostringstream sql;
  sql << "UPDATE widget SET name = "
      << DataFactory::getNullableStringColumnValue(entry.getName())
      << ", category = "
      << DataFactory::getNullableStringColumnValue(entry.getCategory())
      << ", quantity = " << entry.getQuantity()
      << " WHERE id = " << entry.getId();
  return sql.str();
}

std::string
WidgetTable::buildDeleteSql(const WidgetKey& key)
{
  std::ostringstream sql;
  sql << "DELETE FROM widget WHERE id = " << widgetId(key);
  return sql.str();
}

std::string
WidgetTable::buildSelectByKeySql(const WidgetKey& key)
{
  std::ostringstream sql;
  sql << "SELECT id, name, category, quantity FROM widget WHERE id = " << widgetId(key);
  return sql.str();
}

std::string
WidgetTable::buildSelectAllSql()
{
  return "SELECT id, name, category, quantity FROM widget";
}

std::string
WidgetTable::buildSelectByCategorySql(const std::string& category)
{
  std::ostringstream sql;
  sql << "SELECT id, name, category, quantity FROM widget WHERE category = "
      << DataFactory::getNullableStringColumnValue(category);
  return sql.str();
}

WidgetEntry
WidgetTable::RowMapperImpl::mapRow(MYSQL_ROW row, unsigned long* /*lengths*/) const
{
  WidgetEntry entry;
  entry.setKey(WidgetKey{DataFactory::fetchNullableIntegerColumnValue(row[0]).value_or(0)});
  entry.setName(DataFactory::fetchNullableStringColumnValue(row[1]).value_or(""));
  entry.setCategory(DataFactory::fetchNullableStringColumnValue(row[2]).value_or(""));
  entry.setQuantity(DataFactory::fetchNullableIntegerColumnValue(row[3]).value_or(0));
  return entry;
}

} // namespace ModernCommon
