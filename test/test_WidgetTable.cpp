/* -*- C++ -*- */
//
// test_WidgetTable.cpp
//
// Unit tests for WidgetTable.h -- the worked example of a concrete
// Entry<>/Table<> table keyed by std::tuple<int32_t>. Covers the SQL
// string builders and RowMapperImpl::mapRow() directly (both are pure
// functions, no I/O), plus confirms insert()/update()/remove()/query()
// reach DataFactory correctly by observing them fail with
// ServiceUnavailable on an unconnected table -- none of it needs a live
// MySQL server.

#include "DataError.h"
#include "MiniTest.h"
#include "WidgetTable.h"

using namespace ModernCommon;

namespace {

void test_WidgetKeyIsATuple(MiniTest& t)
{
  WidgetKey key{42};
  MT_CHECK(t, widgetId(key) == 42);
  MT_CHECK(t, key == WidgetKey{42});
  MT_CHECK(t, WidgetKey{1} < WidgetKey{2});
}

void test_WidgetEntryAccessorsAndGetString(MiniTest& t)
{
  WidgetEntry entry(WidgetKey{1}, "bolt", "hardware", 10);
  MT_CHECK(t, entry.getId() == 1);
  MT_CHECK(t, entry.getName() == "bolt");
  MT_CHECK(t, entry.getCategory() == "hardware");
  MT_CHECK(t, entry.getQuantity() == 10);

  entry.setName("nut");
  entry.setCategory("fasteners");
  entry.setQuantity(20);
  MT_CHECK(t, entry.getName() == "nut");
  MT_CHECK(t, entry.getCategory() == "fasteners");
  MT_CHECK(t, entry.getQuantity() == 20);

  MT_CHECK(t, entry.getString() ==
              "[ WidgetEntry: id=1, name=nut, "
              "category=fasteners, quantity=20 ]");
}

void test_BuildInsertSql(MiniTest& t)
{
  WidgetEntry entry(WidgetKey{1}, "bolt", "hardware", 10);
  MT_CHECK(t, WidgetTable::buildInsertSql(entry) ==
              "INSERT INTO widget (id, name, category, quantity) VALUES "
              "(1, 'bolt', 'hardware', 10)");
}

void test_BuildInsertSqlEscapesName(MiniTest& t)
{
  WidgetEntry entry(WidgetKey{2}, "O'Brien's bolt", "hardware", 5);
  MT_CHECK(t, WidgetTable::buildInsertSql(entry) ==
              "INSERT INTO widget (id, name, category, quantity) VALUES "
              "(2, 'O\\'Brien\\'s bolt', 'hardware', 5)");
}

void test_BuildUpdateSql(MiniTest& t)
{
  WidgetEntry entry(WidgetKey{1}, "bolt", "hardware", 15);
  MT_CHECK(t, WidgetTable::buildUpdateSql(entry) ==
              "UPDATE widget SET name = 'bolt', category = 'hardware', "
              "quantity = 15 WHERE id = 1");
}

void test_BuildDeleteSql(MiniTest& t)
{
  MT_CHECK(t, WidgetTable::buildDeleteSql(WidgetKey{3}) ==
              "DELETE FROM widget WHERE id = 3");
}

void test_BuildSelectByKeySql(MiniTest& t)
{
  MT_CHECK(t, WidgetTable::buildSelectByKeySql(WidgetKey{4}) ==
              "SELECT id, name, category, quantity FROM widget WHERE id = 4");
}

void test_BuildSelectAllSql(MiniTest& t)
{
  MT_CHECK(t, WidgetTable::buildSelectAllSql() ==
              "SELECT id, name, category, quantity FROM widget");
}

void test_BuildSelectByCategorySql(MiniTest& t)
{
  MT_CHECK(t, WidgetTable::buildSelectByCategorySql("hardware") ==
              "SELECT id, name, category, quantity FROM widget "
              "WHERE category = 'hardware'");
}

void test_RowMapperMapsRowToEntry(MiniTest& t)
{
  char idCell[] = "5";
  char nameCell[] = "washer";
  char categoryCell[] = "hardware";
  char quantityCell[] = "30";
  char* cells[4] = { idCell, nameCell, categoryCell, quantityCell };

  WidgetTable::RowMapperImpl mapper;
  WidgetEntry entry = mapper.mapRow(cells, nullptr);

  MT_CHECK(t, entry.getId() == 5);
  MT_CHECK(t, entry.getName() == "washer");
  MT_CHECK(t, entry.getCategory() == "hardware");
  MT_CHECK(t, entry.getQuantity() == 30);
}

void test_RowMapperHandlesSqlNullNameAndQuantity(MiniTest& t)
{
  char idCell[] = "6";
  char* cells[4] = { idCell, nullptr, nullptr, nullptr }; // SQL NULL name/category/quantity

  WidgetTable::RowMapperImpl mapper;
  WidgetEntry entry = mapper.mapRow(cells, nullptr);

  MT_CHECK(t, entry.getId() == 6);
  MT_CHECK(t, entry.getName() == "");
  MT_CHECK(t, entry.getCategory() == "");
  MT_CHECK(t, entry.getQuantity() == 0);
}

void test_InsertWithoutConnectionThrows(MiniTest& t)
{
  WidgetTable table;
  bool threw = false;
  try { table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10)); }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_UpdateWithoutConnectionThrows(MiniTest& t)
{
  WidgetTable table;
  bool threw = false;
  try { table.update(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10)); }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_RemoveWithoutConnectionThrows(MiniTest& t)
{
  WidgetTable table;
  bool threw = false;
  try { table.remove(WidgetKey{1}); }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_QueryByKeyWithoutConnectionThrows(MiniTest& t)
{
  WidgetTable table;
  bool threw = false;
  try
  {
    table.query(WidgetKey{1}, [](const WidgetEntry&) { return true; });
  }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_QueryAllWithoutConnectionThrows(MiniTest& t)
{
  WidgetTable table;
  bool threw = false;
  try
  {
    table.query([](const WidgetEntry&) { return true; });
  }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_QueryByCategoryWithoutConnectionThrows(MiniTest& t)
{
  WidgetTable table;
  bool threw = false;
  try
  {
    table.query(std::string("hardware"), [](const WidgetEntry&) { return true; });
  }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

} // namespace

int main()
{
  MiniTest t("test_WidgetTable");

  MT_RUN(t, test_WidgetKeyIsATuple);
  MT_RUN(t, test_WidgetEntryAccessorsAndGetString);
  MT_RUN(t, test_BuildInsertSql);
  MT_RUN(t, test_BuildInsertSqlEscapesName);
  MT_RUN(t, test_BuildUpdateSql);
  MT_RUN(t, test_BuildDeleteSql);
  MT_RUN(t, test_BuildSelectByKeySql);
  MT_RUN(t, test_BuildSelectAllSql);
  MT_RUN(t, test_BuildSelectByCategorySql);
  MT_RUN(t, test_RowMapperMapsRowToEntry);
  MT_RUN(t, test_RowMapperHandlesSqlNullNameAndQuantity);
  MT_RUN(t, test_InsertWithoutConnectionThrows);
  MT_RUN(t, test_UpdateWithoutConnectionThrows);
  MT_RUN(t, test_RemoveWithoutConnectionThrows);
  MT_RUN(t, test_QueryByKeyWithoutConnectionThrows);
  MT_RUN(t, test_QueryAllWithoutConnectionThrows);
  MT_RUN(t, test_QueryByCategoryWithoutConnectionThrows);

  return t.result();
}
