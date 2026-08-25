/* -*- C++ -*- */
//
// test_Data.cpp
//
// Unit tests for the backend-agnostic layer in Data.h: Entry<>, the
// RowCallback-based query helpers (findByKey/queryAll/countAll), and
// Table<>::dump(). None of this needs a real database -- a fake,
// in-memory table (FakeWidgetTable) stands in for a MySQL-backed one so
// every piece here is exercised without libmysqlclient making a single
// network call. Compare with MyTemplate's test_Data.cpp: what used to be
// five ResultsProcessor<> subclasses under test is now three free
// functions plus ad hoc lambdas -- see Data.h's file comment.

#include "Data.h"
#include "MiniTest.h"
#include "WidgetTable.h"
#include <algorithm>
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

/// Stand-in for DataFactory: satisfies Table<>'s
/// `ConnectionPolicy(tableName)` constructor requirement and
/// getTableName() call, with no MySQL involved at all.
class FakeDataFactory
{
public:
  explicit FakeDataFactory(std::string tableName) : _tableName(std::move(tableName)) {}
  virtual ~FakeDataFactory() = default;
  const std::string& getTableName() const { return _tableName; }

private:
  std::string _tableName;
};

/// An in-memory Table<>: query() iterates a std::vector instead of
/// running SQL, remove() erases from it. This drives every piece in
/// Data.h without a database.
class FakeWidgetTable : public Table<WidgetKey, WidgetEntry, FakeDataFactory>
{
public:
  FakeWidgetTable() : Table<WidgetKey, WidgetEntry, FakeDataFactory>("widget") {}
  ~FakeWidgetTable() override = default;

  // Same reasoning as WidgetTable's (WidgetTable.h): this class's own
  // destructor above suppresses implicit move generation, and
  // makeThreeWidgetTable() below returns one by value.
  FakeWidgetTable(const FakeWidgetTable&) = delete;
  FakeWidgetTable& operator=(const FakeWidgetTable&) = delete;
  FakeWidgetTable(FakeWidgetTable&&) = default;
  FakeWidgetTable& operator=(FakeWidgetTable&&) = default;

  std::vector<WidgetEntry> entries;

  // Both overloads iterate a snapshot copy, not `entries` itself, so a
  // callback that mutates the table mid-query doesn't invalidate the
  // iteration -- the same guarantee a real SELECT's result set gives you
  // against concurrent DELETEs.

  void query(const WidgetKey& key, const RowCallback<WidgetEntry>& cb) override
  {
    std::vector<WidgetEntry> snapshot = entries;
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
    std::vector<WidgetEntry> snapshot = entries;
    for (const auto& e : snapshot)
    {
      if (!cb(e)) break;
    }
  }

  void remove(const WidgetKey& key)
  {
    entries.erase(
      std::remove_if(entries.begin(), entries.end(),
                      [&](const WidgetEntry& e) { return e.getKey() == key; }),
      entries.end());
  }
};

FakeWidgetTable makeThreeWidgetTable()
{
  FakeWidgetTable table;
  table.entries.push_back(WidgetEntry(WidgetKey{1}, "bolt", "hardware", 10));
  table.entries.push_back(WidgetEntry(WidgetKey{2}, "nut", "hardware", 20));
  table.entries.push_back(WidgetEntry(WidgetKey{3}, "washer", "hardware", 30));
  return table;
}

// ------------------------------------------------------------------
// Entry<>
// ------------------------------------------------------------------

void test_EntryDefaultAndKeyConstruction(MiniTest& t)
{
  Entry<WidgetKey> defaulted;
  MT_CHECK(t, defaulted.getKey() == WidgetKey{0});

  Entry<WidgetKey> withKey(WidgetKey{7});
  MT_CHECK(t, withKey.getKey() == WidgetKey{7});
}

void test_EntrySetKey(MiniTest& t)
{
  Entry<WidgetKey> entry;
  entry.setKey(WidgetKey{9});
  MT_CHECK(t, entry.getKey() == WidgetKey{9});
}

// ------------------------------------------------------------------
// RowCallback-based query helpers (findByKey/queryAll/countAll)
// ------------------------------------------------------------------

void test_QueryAll(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  std::vector<WidgetEntry> copy = queryAll(table);

  MT_CHECK(t, copy.size() == 3);
  MT_CHECK(t, copy[0].getKey() == WidgetKey{1});
  MT_CHECK(t, copy[2].getName() == "washer");
}

void test_FindByKeyMatchFound(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  std::optional<WidgetEntry> found = findByKey(table, WidgetKey{2});

  MT_CHECK(t, found.has_value());
  MT_CHECK(t, found->getName() == "nut");
}

void test_FindByKeyNoMatch(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  std::optional<WidgetEntry> found = findByKey(table, WidgetKey{999});
  MT_CHECK(t, !found.has_value());
}

void test_CountAll(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();
  MT_CHECK(t, countAll(table) == 3);
}

void test_DeleteViaLambda(MiniTest& t)
{
  // No DeleteProcessorTemplate-shaped class needed -- a lambda at the
  // call site does the job (see Data.h's file comment).
  FakeWidgetTable table = makeThreeWidgetTable();

  std::vector<WidgetKey> toDelete;
  table.query([&](const WidgetEntry& e) {
    toDelete.push_back(e.getKey());
    return true;
  });
  for (const auto& key : toDelete) table.remove(key);

  MT_CHECK(t, table.entries.empty());
}

// ------------------------------------------------------------------
// Table<>::dump()
// ------------------------------------------------------------------

void test_TableDump(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  std::string output;
  {
    CoutCapture capture;
    table.dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("widget Dump:") != std::string::npos);
  MT_CHECK(t, output.find("bolt") != std::string::npos);
  MT_CHECK(t, output.find("nut") != std::string::npos);
  MT_CHECK(t, output.find("washer") != std::string::npos);
}

} // namespace

int main()
{
  MiniTest t("test_Data");

  MT_RUN(t, test_EntryDefaultAndKeyConstruction);
  MT_RUN(t, test_EntrySetKey);
  MT_RUN(t, test_QueryAll);
  MT_RUN(t, test_FindByKeyMatchFound);
  MT_RUN(t, test_FindByKeyNoMatch);
  MT_RUN(t, test_CountAll);
  MT_RUN(t, test_DeleteViaLambda);
  MT_RUN(t, test_TableDump);

  return t.result();
}
