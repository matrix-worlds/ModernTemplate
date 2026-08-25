/* -*- C++ -*- */
//
// Data.h
//
// Modern-C++ replacement for MyTemplate's Data.h: a row base type, a
// generic key+row wrapper, and the abstract Table<> every concrete
// table derives from.
//
// The biggest change from MyTemplate is what happened to
// ResultsProcessor<T> and its five subclasses (CopyProcessor,
// FindProcessor, CountProcessorTemplate, DeleteProcessorTemplate,
// CommonDumpProcessor): a query callback is now just a
// `std::function<bool(const Entry&)>` (RowCallback<Entry> below) --
// return false to stop early, same contract as before. Writing a whole
// class per query shape (copy results out, find the first match, count
// matches, ...) was solving a problem C++98 had and C++11 lambdas
// don't: those five classes collapse into the three tiny free function
// templates at the bottom of this file (findByKey/queryAll/countAll),
// each a one-line lambda over the single callback type. Anything not
// covered by those three -- deleting every matched row, say -- is just
// a lambda written at the call site; there's no need for a
// DeleteProcessorTemplate-shaped class to exist at all.
//
// CommonTable<>'s ad hoc, never-actually-Dumpable virtual dump() is
// kept the same way MyTemplate kept it (see that project's file
// comment for why): it's still not tied to Dump.h's
// DiagnosticDumpRegistry, to stay a faithful, minimal-surprise port of
// the original design's two-separate-dump-mechanisms quirk.

#ifndef MODERNTEMPLATE_DATA_H
#define MODERNTEMPLATE_DATA_H

#include "DataFactory.h"
#include "Stringable.h"
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ModernCommon {

  /// Base class for row/entry types. Kept Stringable (unlike
  /// ModernType<>, see that file's comment) because CommonDumpProcessor's
  /// modern equivalent -- the dump() lambda below -- and CacheData.h's
  /// snapshot cache both need to print an Entry through a generic,
  /// non-templated code path.
  class Data : public Stringable
  {
  public:
    std::string getString() const override { return "Data::getString()"; }
  };

  /// A row: a Key plus whatever Base contributes.
  template<class Key, class Base = Data>
  class Entry : public Base
  {
  public:
    Entry() = default;
    explicit Entry(Key key) : _key(std::move(key)) {}

    const Key& getKey() const { return _key; }
    void setKey(Key key) { _key = std::move(key); }

  protected:
    Key _key{};
  };

  /// The single query callback type, replacing MyTemplate's
  /// ResultsProcessor<T> class hierarchy -- see the file comment.
  template<class EntryT>
  using RowCallback = std::function<bool(const EntryT&)>;

  /**
   * Table
   *
   * Common interface every concrete database table implements: query
   * with a key, query without one, and (provided here) dump().
   * ConnectionPolicy defaults to DataFactory but can be swapped out
   * (e.g. by a test double).
   */
  template<class Key, class EntryT, class ConnectionPolicy = DataFactory>
  class Table : public ConnectionPolicy
  {
  public:
    explicit Table(std::string tableName) : ConnectionPolicy(std::move(tableName)) {}

    ~Table() override = default;

    /// Move-only, matching DataFactory's own contract -- see that
    /// file's comment. Has to be spelled out here too: this class's own
    /// (defaulted, still user-declared) destructor above suppresses
    /// implicit generation of its move members.
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&&) = default;
    Table& operator=(Table&&) = default;

    /// Query constrained to the row identified by `key`.
    virtual void query(const Key& key, const RowCallback<EntryT>& cb) = 0;

    /// Unconstrained query: every row in the table.
    virtual void query(const RowCallback<EntryT>& cb) = 0;

    virtual void dump()
    {
      std::cout << this->getTableName() << " Dump:\n"
                << "===================================================\n";
      query([](const EntryT& e) {
        std::cout << e.getString() << "\n";
        return true;
      });
      std::cout << "===================================================\n";
    }
  };

  /**
   * runQuery
   *
   * Drives a SELECT through to completion against `df`: executes `sql`,
   * converts each fetched row via `mapper`, and feeds it to `cb`,
   * stopping early if that returns false. Always frees the MySQL result
   * set, including when process()/mapRow() throws. Same role as
   * MyTemplate's runQuery(), just taking a RowCallback instead of a
   * ResultsProcessor&.
   */
  template<class EntryT>
  void runQuery(DataFactory& df,
                const std::string& sql,
                const RowMapper<EntryT>& mapper,
                const RowCallback<EntryT>& cb)
  {
    MYSQL_RES* result = df.executeQuery(sql);
    if (result == nullptr) return;

    try
    {
      bool more = true;
      MYSQL_ROW row;
      while (more && (row = mysql_fetch_row(result)) != nullptr)
      {
        unsigned long* lengths = mysql_fetch_lengths(result);
        more = cb(mapper.mapRow(row, lengths));
      }
    }
    catch (...)
    {
      mysql_free_result(result);
      throw;
    }
    mysql_free_result(result);
  }

  // -- Free-function query helpers, replacing MyTemplate's
  //    ResultsProcessor<> subclasses --

  /// The first row matching `key`, or std::nullopt if there isn't one.
  /// Replaces FindProcessor<>.
  template<class Key, class EntryT, class ConnectionPolicy>
  std::optional<EntryT> findByKey(Table<Key, EntryT, ConnectionPolicy>& table, const Key& key)
  {
    std::optional<EntryT> found;
    table.query(key, [&](const EntryT& e) {
      found = e;
      return false; // one match is enough
    });
    return found;
  }

  /// Every row in the table. Replaces CopyProcessor<>.
  template<class Key, class EntryT, class ConnectionPolicy>
  std::vector<EntryT> queryAll(Table<Key, EntryT, ConnectionPolicy>& table)
  {
    std::vector<EntryT> results;
    table.query([&](const EntryT& e) {
      results.push_back(e);
      return true;
    });
    return results;
  }

  /// Number of rows in the table. Replaces CountProcessorTemplate<>.
  template<class Key, class EntryT, class ConnectionPolicy>
  std::size_t countAll(Table<Key, EntryT, ConnectionPolicy>& table)
  {
    std::size_t n = 0;
    table.query([&](const EntryT&) {
      ++n;
      return true;
    });
    return n;
  }

} // namespace ModernCommon

#endif // MODERNTEMPLATE_DATA_H
