/* -*- C++ -*- */
//
// WidgetTable.h
//
// A worked example of the Entry<>/Table<> pattern from Data.h, backed
// by the same MySQL schema as MyTemplate's WidgetTable.h so the two
// projects can be compared side by side:
//
//   CREATE TABLE widget (
//     id       INT PRIMARY KEY,
//     name     VARCHAR(255) NOT NULL,
//     category VARCHAR(255) NOT NULL,
//     quantity INT NOT NULL DEFAULT 0
//   );
//
// The key modernization here: WidgetKey is a plain std::tuple<int32_t>
// instead of a hand-written CommonKey1<int32> subclass. A single-column
// key needed a whole class (constructor, getA()/setA(), getString(),
// operator<, operator==, ...) in MyTemplate purely so CommonMap<>'s
// std::map could order it and CacheData.h could print it; std::tuple
// already provides lexicographic <=>/== for any arity for free, and a
// tuple with one element still reads clearly as "the widget id" at each
// call site (`WidgetKey{id}` / `std::get<0>(key)`). See CommonKey1..6 in
// MyTemplate's Data.h for the six hand-rolled classes this one type
// alias replaces.
//
// `category` and the constrained query(category, cb) overload exist to
// give CacheData.h's CacheData<> (whose Constraint here is std::string)
// something real to filter by, mirroring MyTemplate's category-filtered
// WidgetTable exactly.
//
// The SQL-building methods stay `static` and public specifically so
// test/test_WidgetTable.cpp can verify generated SQL text, and
// RowMapperImpl::mapRow() can be exercised against a hand-built
// MYSQL_ROW, without either needing a live MySQL server -- same
// rationale as MyTemplate's.

#ifndef MODERNTEMPLATE_WIDGETTABLE_H
#define MODERNTEMPLATE_WIDGETTABLE_H

#include "Data.h"
#include <cstdint>
#include <sstream>
#include <tuple>

namespace ModernCommon {

  /// A widget's key is just its id. std::tuple<> gives us comparison
  /// operators for free -- see the file comment.
  using WidgetKey = std::tuple<std::int32_t>;

  inline std::int32_t widgetId(const WidgetKey& key) { return std::get<0>(key); }

  class WidgetEntry : public Entry<WidgetKey>
  {
  public:
    WidgetEntry() = default;

    WidgetEntry(WidgetKey key, std::string name, std::string category, int quantity)
      : Entry<WidgetKey>(std::move(key)),
        _name(std::move(name)),
        _category(std::move(category)),
        _quantity(quantity)
    {}

    std::int32_t getId() const { return widgetId(getKey()); }

    const std::string& getName() const { return _name; }
    void setName(std::string name) { _name = std::move(name); }

    const std::string& getCategory() const { return _category; }
    void setCategory(std::string category) { _category = std::move(category); }

    int getQuantity() const { return _quantity; }
    void setQuantity(int quantity) { _quantity = quantity; }

    std::string getString() const override
    {
      std::ostringstream ost;
      ost << "[ WidgetEntry: id=" << getId()
          << ", name=" << _name
          << ", category=" << _category
          << ", quantity=" << _quantity
          << " ]";
      return ost.str();
    }

  private:
    std::string _name;
    std::string _category;
    int _quantity = 0;
  };

  class WidgetTable : public Table<WidgetKey, WidgetEntry, DataFactory>
  {
  public:
    WidgetTable() : Table<WidgetKey, WidgetEntry, DataFactory>("widget") {}
    ~WidgetTable() override = default;

    /// Same reasoning as Table<>'s own move-only declarations (Data.h):
    /// this class's own destructor above again suppresses implicit move
    /// generation, so it has to be re-stated here too.
    WidgetTable(const WidgetTable&) = delete;
    WidgetTable& operator=(const WidgetTable&) = delete;
    WidgetTable(WidgetTable&&) = default;
    WidgetTable& operator=(WidgetTable&&) = default;

    void insert(const WidgetEntry& entry) { executeModificationSql(buildInsertSql(entry)); }
    void update(const WidgetEntry& entry) { executeModificationSql(buildUpdateSql(entry)); }
    void remove(const WidgetKey& key) { executeModificationSql(buildDeleteSql(key)); }

    void query(const WidgetKey& key, const RowCallback<WidgetEntry>& cb) override
    {
      RowMapperImpl mapper;
      runQuery<WidgetEntry>(*this, buildSelectByKeySql(key), mapper, cb);
    }

    void query(const RowCallback<WidgetEntry>& cb) override
    {
      RowMapperImpl mapper;
      runQuery<WidgetEntry>(*this, buildSelectAllSql(), mapper, cb);
    }

    /// Constrained query: only widgets in `category`. This is what
    /// CacheData<WidgetKey, WidgetEntry, std::string>::reload() drives
    /// through, one category at a time.
    void query(const std::string& category, const RowCallback<WidgetEntry>& cb)
    {
      RowMapperImpl mapper;
      runQuery<WidgetEntry>(*this, buildSelectByCategorySql(category), mapper, cb);
    }

    // SQL builders: pure functions of their arguments, no I/O. Public so
    // they can be unit tested directly.
    static std::string buildInsertSql(const WidgetEntry& entry);
    static std::string buildUpdateSql(const WidgetEntry& entry);
    static std::string buildDeleteSql(const WidgetKey& key);
    static std::string buildSelectByKeySql(const WidgetKey& key);
    static std::string buildSelectAllSql();
    static std::string buildSelectByCategorySql(const std::string& category);

    /// Column order for both the SELECT lists above and mapRow() below:
    /// id, name, category, quantity.
    class RowMapperImpl : public RowMapper<WidgetEntry>
    {
    public:
      WidgetEntry mapRow(MYSQL_ROW row, unsigned long* lengths) const override;
    };
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_WIDGETTABLE_H
