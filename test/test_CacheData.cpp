/* -*- C++ -*- */
//
// test_CacheData.cpp
//
// Unit tests for CacheData.h -- the last link in the chain this project
// has built: ModernType<> -> Synchronized.h -> Data.h -> CacheData<>.
// None of this needs a live MySQL server: FakeWidgetTable (below) stands
// in for WidgetTable, backed by a static in-memory "database" every
// fresh `Table table;` CacheData<>::reload() constructs can see -- the
// same way every fresh real WidgetTable sees the same live MySQL server.
//
// See test_Integration.cpp for the same coverage against a real MySQL
// server.

#include "CacheData.h"
#include "MiniTest.h"
#include "WidgetTable.h"
#include <sstream>
#include <streambuf>
#include <vector>

using namespace ModernCommon;

namespace {

/// Stand-in for DataFactory (same role as in test_Data.cpp).
class FakeDataFactory
{
public:
  explicit FakeDataFactory(std::string tableName) : _tableName(std::move(tableName)) {}
  virtual ~FakeDataFactory() = default;
  const std::string& getTableName() const { return _tableName; }

private:
  std::string _tableName;
};

/// A static, process-wide "database" every fresh FakeWidgetTable
/// instance reads from -- mirroring how every fresh real WidgetTable
/// instance talks to the same live MySQL server. Each test resets it.
class FakeWidgetDatabase
{
public:
  static std::vector<WidgetEntry>& rows()
  {
    static std::vector<WidgetEntry> data;
    return data;
  }

  static void reset(const std::vector<WidgetEntry>& seed) { rows() = seed; }
};

class FakeWidgetTable : public Table<WidgetKey, WidgetEntry, FakeDataFactory>
{
public:
  FakeWidgetTable() : Table<WidgetKey, WidgetEntry, FakeDataFactory>("widget") {}
  ~FakeWidgetTable() override = default;

  FakeWidgetTable(const FakeWidgetTable&) = delete;
  FakeWidgetTable& operator=(const FakeWidgetTable&) = delete;
  FakeWidgetTable(FakeWidgetTable&&) = default;
  FakeWidgetTable& operator=(FakeWidgetTable&&) = default;

  void query(const WidgetKey& key, const RowCallback<WidgetEntry>& cb) override
  {
    std::vector<WidgetEntry> snapshot = FakeWidgetDatabase::rows();
    for (const auto& e : snapshot)
    {
      if (e.getKey() == key)
      {
        if (!cb(e)) break;
      }
    }
  }

  void query(const RowCallback<WidgetEntry>& cb) override
  {
    std::vector<WidgetEntry> snapshot = FakeWidgetDatabase::rows();
    for (const auto& e : snapshot)
    {
      if (!cb(e)) break;
    }
  }

  /// The constrained query CacheData<...,std::string>::reload() calls:
  /// only rows in `category`.
  void query(const std::string& category, const RowCallback<WidgetEntry>& cb)
  {
    std::vector<WidgetEntry> snapshot = FakeWidgetDatabase::rows();
    for (const auto& e : snapshot)
    {
      if (e.getCategory() == category)
      {
        if (!cb(e)) break;
      }
    }
  }
};

using WidgetCacheData = CacheData<WidgetKey, WidgetEntry>;

void seedThreeWidgetsTwoCategories()
{
  std::vector<WidgetEntry> data;
  data.push_back(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  data.push_back(WidgetEntry(WidgetKey{2}, "nut", "hardware", 20));
  data.push_back(WidgetEntry(WidgetKey{3}, "screwdriver", "tools", 5));
  FakeWidgetDatabase::reset(data);
}

// ------------------------------------------------------------------
// Unconstrained use: CacheData<WidgetKey, WidgetEntry>
// ------------------------------------------------------------------

void test_CacheDataConstruction(MiniTest& t)
{
  WidgetCacheData cache;
  MT_CHECK(t, cache.size() == 0); // empty until reload()
}

void test_CacheDataReloadPopulatesFromTable(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  FakeWidgetTable table;
  WidgetCacheData cache;
  cache.reload(table);

  MT_CHECK(t, cache.size() == 3);
}

void test_CacheDataLookupReturnsSharedPtr(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  FakeWidgetTable table;
  WidgetCacheData cache;
  cache.reload(table);

  auto found = cache.lookup(WidgetKey{2});
  MT_CHECK(t, found != nullptr);
  MT_CHECK(t, found->getName() == "nut");

  MT_CHECK(t, cache.lookup(WidgetKey{999}) == nullptr);
}

/// reload() rebuilds into a private local SynchronizedMap and swaps it
/// in atomically (Synchronized.h's reload()) -- a second reload() after
/// the underlying data changed must reflect the new data, not an
/// accumulation of both.
void test_CacheDataReloadReflectsLatestData(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  FakeWidgetTable table;
  WidgetCacheData cache;
  cache.reload(table);
  MT_CHECK(t, cache.size() == 3);

  std::vector<WidgetEntry> updated;
  updated.push_back(WidgetEntry(WidgetKey{4}, "hammer", "tools", 2));
  FakeWidgetDatabase::reset(updated);

  cache.reload(table);
  MT_CHECK(t, cache.size() == 1);

  MT_CHECK(t, cache.lookup(WidgetKey{4}) != nullptr);
  MT_CHECK(t, cache.lookup(WidgetKey{1}) == nullptr); // gone after reload
}

void test_CacheDataLookupAllReturnsEveryRow(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  FakeWidgetTable table;
  WidgetCacheData cache;
  cache.reload(table);

  auto all = cache.lookupAll();
  MT_CHECK(t, all.size() == 3);
}

/// A lookup()'d shared_ptr stays valid across a reload() that evicts it
/// from the cache -- unlike a raw pointer into a swapped-out map, the
/// caller's copy of the shared_ptr keeps the object alive.
void test_CacheDataLookupSurvivesReload(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  FakeWidgetTable table;
  WidgetCacheData cache;
  cache.reload(table);

  auto bolt = cache.lookup(WidgetKey{1});
  MT_CHECK(t, bolt != nullptr);

  FakeWidgetDatabase::reset({});
  cache.reload(table);
  MT_CHECK(t, cache.size() == 0);

  MT_CHECK(t, bolt->getName() == "bolt"); // still valid
}

// ------------------------------------------------------------------
// Constrained use: CacheData<WidgetKey, WidgetEntry, std::string>
// ------------------------------------------------------------------

using WidgetCategoryCacheData = CacheData<WidgetKey, WidgetEntry, std::string>;

WidgetCategoryCacheData makeCategoryCache()
{
  return WidgetCategoryCacheData(
    [](const std::string& category, const WidgetEntry& e) { return e.getCategory() == category; });
}

void test_ConstrainedCacheDataFiltersByPredicate(MiniTest& t)
{
  seedThreeWidgetsTwoCategories(); // 2 hardware, 1 tools

  FakeWidgetTable table;
  WidgetCategoryCacheData cache = makeCategoryCache();
  cache.reload(table); // caches every row; filtering happens at lookupAll()

  auto hardware = cache.lookupAll(std::string("hardware"));
  MT_CHECK(t, hardware.size() == 2);

  auto tools = cache.lookupAll(std::string("tools"));
  MT_CHECK(t, tools.size() == 1);
  MT_CHECK(t, tools.front()->getName() == "screwdriver");
}

} // namespace

int main()
{
  MiniTest t("test_CacheData");

  MT_RUN(t, test_CacheDataConstruction);
  MT_RUN(t, test_CacheDataReloadPopulatesFromTable);
  MT_RUN(t, test_CacheDataLookupReturnsSharedPtr);
  MT_RUN(t, test_CacheDataReloadReflectsLatestData);
  MT_RUN(t, test_CacheDataLookupAllReturnsEveryRow);
  MT_RUN(t, test_CacheDataLookupSurvivesReload);
  MT_RUN(t, test_ConstrainedCacheDataFiltersByPredicate);

  return t.result();
}
