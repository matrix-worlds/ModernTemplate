/* -*- C++ -*- */
//
// test_Integration.cpp
//
// End-to-end test of the whole chain this project set out to validate,
// against a *real* MySQL server:
//
//   ModernType<>       (not used by WidgetKey directly -- see WidgetTable.h)
//   -> WidgetKey (std::tuple<int32_t>)
//   -> Entry<>/Table<>       (WidgetEntry / WidgetTable)
//   -> CacheData<>           (CacheData.h)
//
// i.e. rows really inserted through WidgetTable into MySQL, really
// fetched back out via WidgetTable::query()/findByKey()/queryAll()/
// countAll(), really cached as std::shared_ptr<WidgetEntry> in a
// CacheData<>, really looked back up by key -- covering in one file what
// MyTemplate split across test_WidgetTable_Integration.cpp and
// test_CacheData_Integration.cpp, since ModernTemplate's smaller surface
// (no separate Constrained/Unconstrained cache types, no ResultsProcessor
// hierarchy) makes one combined suite easy to follow.
//
// Requires a MySQL server reachable with the connection parameters below
// and a `widget` table already created in that database (see
// WidgetTable.h's file comment for the CREATE TABLE statement; doc/ has
// the full setup instructions). If no server is reachable, main() prints
// a SKIPPED notice and exits 0 rather than failing -- this test suite is
// not meant to force every environment to run a MySQL server just to
// build this project.
//
// Connection parameters are read from environment variables so this
// isn't hardwired to one developer's local setup:
//   MODERNTEMPLATE_MYSQL_HOST     (default "127.0.0.1")
//   MODERNTEMPLATE_MYSQL_PORT     (default 3306)
//   MODERNTEMPLATE_MYSQL_USER     (default "root")
//   MODERNTEMPLATE_MYSQL_PASSWORD (default "")
//   MODERNTEMPLATE_MYSQL_DATABASE (default "moderntemplate_test")
//
// A separate database/env-var prefix from MyTemplate's
// (mytemplate_test / MYTEMPLATE_MYSQL_*) is deliberate: the two projects
// are independent worked examples meant to be run and compared side by
// side, not sharing mutable state.

#include "CacheData.h"
#include "DataError.h"
#include "MiniTest.h"
#include "WidgetTable.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <vector>

using namespace ModernCommon;

namespace {

/// Redirects std::cout into a string for the duration of its scope.
class CoutCapture
{
public:
  CoutCapture() : _old(std::cout.rdbuf(_buf.rdbuf())) {}
  ~CoutCapture() { std::cout.rdbuf(_old); }
  std::string str() const { return _buf.str(); }

private:
  std::ostringstream _buf;
  std::streambuf* _old;
};

std::string envOr(const char* name, const std::string& fallback)
{
  const char* v = std::getenv(name);
  return (v != nullptr && v[0] != '\0') ? std::string(v) : fallback;
}

MySqlConnectionParams testConnectionParams()
{
  MySqlConnectionParams params;
  params.host = envOr("MODERNTEMPLATE_MYSQL_HOST", "127.0.0.1");
  params.user = envOr("MODERNTEMPLATE_MYSQL_USER", "root");
  params.password = envOr("MODERNTEMPLATE_MYSQL_PASSWORD", "");
  params.database = envOr("MODERNTEMPLATE_MYSQL_DATABASE", "moderntemplate_test");
  params.port = static_cast<unsigned int>(
    std::atoi(envOr("MODERNTEMPLATE_MYSQL_PORT", "3306").c_str()));
  return params;
}

/// A freshly connect()ed WidgetTable with an empty `widget` table.
WidgetTable freshTable()
{
  WidgetTable table;
  table.connect(testConnectionParams());
  table.executeQuery("DELETE FROM widget"); // doesn't throw on 0 rows affected
  return table;
}

// ------------------------------------------------------------------
// WidgetTable against a real server
// ------------------------------------------------------------------

void test_InsertAndFindByKey(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));

  auto found = findByKey(table, WidgetKey{1});
  MT_CHECK(t, found.has_value());
  MT_CHECK(t, found->getName() == "bolt");
  MT_CHECK(t, found->getQuantity() == 10);
}

void test_FindByKeyNotFound(MiniTest& t)
{
  WidgetTable table = freshTable();
  MT_CHECK(t, !findByKey(table, WidgetKey{999}).has_value());
}

void test_DuplicateInsertThrows(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));

  bool threw = false;
  try { table.insert(WidgetEntry(WidgetKey{1}, "bolt-again", "hardware", 5)); }
  catch (const DuplicateEntry&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_UpdateExistingRow(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.update(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 99));

  auto found = findByKey(table, WidgetKey{1});
  MT_CHECK(t, found.has_value());
  MT_CHECK(t, found->getQuantity() == 99);
}

void test_UpdateNonexistentRowThrows(MiniTest& t)
{
  WidgetTable table = freshTable();

  bool threw = false;
  try { table.update(WidgetEntry(WidgetKey{404}, "nope", "hardware", 1)); }
  catch (const EntryNotFound&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_RemoveRow(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.remove(WidgetKey{1});
  MT_CHECK(t, !findByKey(table, WidgetKey{1}).has_value());
}

void test_RemoveNonexistentRowThrows(MiniTest& t)
{
  WidgetTable table = freshTable();

  bool threw = false;
  try { table.remove(WidgetKey{404}); }
  catch (const EntryNotFound&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_QueryAllAgainstRealTable(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey{2}, "nut", "hardware", 20));
  table.insert(WidgetEntry(WidgetKey{3}, "washer", "hardware", 30));

  MT_CHECK(t, queryAll(table).size() == 3);
}

void test_CountAllAgainstRealTable(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey{2}, "nut", "hardware", 20));

  MT_CHECK(t, countAll(table) == 2);
}

void test_QueryByCategoryAgainstRealTable(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey{2}, "screwdriver", "tools", 5));

  std::vector<WidgetEntry> hardware;
  table.query("hardware", [&](const WidgetEntry& e) {
    hardware.push_back(e);
    return true;
  });
  MT_CHECK(t, hardware.size() == 1);
  MT_CHECK(t, hardware.front().getName() == "bolt");
}

void test_TableDumpAgainstRealData(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));

  std::string output;
  {
    CoutCapture capture;
    table.dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("widget Dump:") != std::string::npos);
  MT_CHECK(t, output.find("bolt") != std::string::npos);
}

void test_TransactionRollbackReverts(MiniTest& t)
{
  WidgetTable table = freshTable();

  MySqlConnection* conn = table.getConnection();
  conn->startTransaction();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  conn->rollbackTransaction();

  MT_CHECK(t, countAll(table) == 0); // rolled back: never persisted
}

void test_TransactionCommitPersists(MiniTest& t)
{
  WidgetTable table = freshTable();

  MySqlConnection* conn = table.getConnection();
  conn->startTransaction();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  conn->commitTransaction();

  MT_CHECK(t, countAll(table) == 1);
}

// ------------------------------------------------------------------
// CacheData<> against a real table, via setDefaultConnectionParams()
// ------------------------------------------------------------------

using WidgetCacheData = CacheData<WidgetKey, WidgetEntry>;
using WidgetCategoryCacheData = CacheData<WidgetKey, WidgetEntry, std::string>;

void test_CacheDataReloadPopulatesFromRealTable(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey{2}, "nut", "hardware", 20));

  WidgetCacheData cache;
  cache.reload(table);

  MT_CHECK(t, cache.size() == 2);

  auto found = cache.lookup(WidgetKey{1});
  MT_CHECK(t, found != nullptr);
  MT_CHECK(t, found->getName() == "bolt");
  MT_CHECK(t, found->getQuantity() == 10);
}

void test_CacheDataLookupMissReturnsNull(MiniTest& t)
{
  WidgetTable table = freshTable(); // empty table

  WidgetCacheData cache;
  cache.reload(table);

  MT_CHECK(t, cache.lookup(WidgetKey{999}) == nullptr);
}

/// Same guarantee as the no-DB test in test_CacheData.cpp, now against a
/// real table: a second reload() after the underlying data actually
/// changed in MySQL reflects the new state, not an accumulation.
void test_CacheDataReloadReflectsLatestRealData(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));

  WidgetCacheData cache;
  cache.reload(table);
  MT_CHECK(t, cache.size() == 1);

  table.remove(WidgetKey{1});
  table.insert(WidgetEntry(WidgetKey{2}, "nut", "hardware", 20));
  table.insert(WidgetEntry(WidgetKey{3}, "washer", "hardware", 30));

  cache.reload(table);
  MT_CHECK(t, cache.size() == 2);

  MT_CHECK(t, cache.lookup(WidgetKey{1}) == nullptr);
  MT_CHECK(t, cache.lookup(WidgetKey{3}) != nullptr);
}

void test_ConstrainedCacheDataFiltersByCategoryAgainstRealTable(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey{2}, "nut", "hardware", 20));
  table.insert(WidgetEntry(WidgetKey{3}, "screwdriver", "tools", 5));

  WidgetCategoryCacheData cache(
    [](const std::string& category, const WidgetEntry& e) { return e.getCategory() == category; });
  cache.reload(table);

  auto hardware = cache.lookupAll(std::string("hardware"));
  MT_CHECK(t, hardware.size() == 2);

  auto tools = cache.lookupAll(std::string("tools"));
  MT_CHECK(t, tools.size() == 1);
  MT_CHECK(t, tools.front()->getName() == "screwdriver");
}

} // namespace

int main()
{
  MySqlConnectionParams params = testConnectionParams();

  // Probe connectivity once up front; skip the whole suite (exit 0, not
  // a failure) if no server is reachable, rather than forcing every
  // build of this project to run a MySQL server.
  try
  {
    WidgetTable probe;
    probe.connect(params);
  }
  catch (const ServiceUnavailable& e)
  {
    std::cout << "[test_Integration] SKIPPED: no MySQL server reachable ("
              << e.what() << ")" << std::endl;
    return 0;
  }

  MiniTest t("test_Integration");

  MT_RUN(t, test_InsertAndFindByKey);
  MT_RUN(t, test_FindByKeyNotFound);
  MT_RUN(t, test_DuplicateInsertThrows);
  MT_RUN(t, test_UpdateExistingRow);
  MT_RUN(t, test_UpdateNonexistentRowThrows);
  MT_RUN(t, test_RemoveRow);
  MT_RUN(t, test_RemoveNonexistentRowThrows);
  MT_RUN(t, test_QueryAllAgainstRealTable);
  MT_RUN(t, test_CountAllAgainstRealTable);
  MT_RUN(t, test_QueryByCategoryAgainstRealTable);
  MT_RUN(t, test_TableDumpAgainstRealData);
  MT_RUN(t, test_TransactionRollbackReverts);
  MT_RUN(t, test_TransactionCommitPersists);
  MT_RUN(t, test_CacheDataReloadPopulatesFromRealTable);
  MT_RUN(t, test_CacheDataLookupMissReturnsNull);
  MT_RUN(t, test_CacheDataReloadReflectsLatestRealData);
  MT_RUN(t, test_ConstrainedCacheDataFiltersByCategoryAgainstRealTable);

  return t.result();
}
