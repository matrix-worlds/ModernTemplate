/* -*- C++ -*- */
//
// DataFactory.h
//
// MySQL-backed replacement for the original DataFactory class -- same
// design MyTemplate arrived at (see that project's DataFactory.h for
// the full ODBC-vs-MySQL rationale), carried over here essentially
// unchanged since talking to the database isn't the "template pattern"
// this project is about modernizing.
//
// Move-only for the same reason as MyTemplate's version: this class
// owns a share of a reference-counted MySqlConnection, so an accidental
// implicit copy (silently possible on any class with a user-declared
// destructor and no explicit copy/move members) would double-release
// that share. See ModernTemplate's Table<> (Data.h) for the same
// requirement one layer up.
//
// One small modernization: fetchNullable*ColumnValue() return
// std::optional<T> instead of MyTemplate's "empty string / zero means
// null" convention -- a genuine SQL NULL is now distinguishable from a
// real empty string or a real zero.

#ifndef MODERNTEMPLATE_DATAFACTORY_H
#define MODERNTEMPLATE_DATAFACTORY_H

#include "DataError.h"
#include "MySqlConnection.h"
#include <cstdlib>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include <mysql.h>

namespace ModernCommon {

  class DataFactory
  {
  public:
    /// If set, every DataFactory constructed afterwards connects
    /// automatically using these params -- lets code that constructs a
    /// Table with no chance to call connect() itself (Table<>'s
    /// default-constructed base, used inside CacheData.h's reload())
    /// still end up connected.
    static void setDefaultConnectionParams(const MySqlConnectionParams& params)
    {
      std::lock_guard<std::mutex> guard(defaultParamsMutex());
      defaultParams() = params;
      haveDefaultParams() = true;
    }

    static void clearDefaultConnectionParams()
    {
      std::lock_guard<std::mutex> guard(defaultParamsMutex());
      haveDefaultParams() = false;
    }

    explicit DataFactory(std::string tableName = "unknown")
      : _tableName(std::move(tableName))
    {
      std::lock_guard<std::mutex> guard(defaultParamsMutex());
      if (haveDefaultParams())
      {
        _connection = MySqlConnection::getCurrentInstance(defaultParams());
      }
    }

    void connect(const MySqlConnectionParams& params)
    {
      _connection = MySqlConnection::getCurrentInstance(params);
    }

    MySqlConnection* getConnection() const { return _connection; }

    virtual ~DataFactory()
    {
      if (_connection != nullptr) _connection->releaseInstance();
    }

    DataFactory(const DataFactory&) = delete;
    DataFactory& operator=(const DataFactory&) = delete;

    DataFactory(DataFactory&& other) noexcept
      : _connection(other._connection), _tableName(std::move(other._tableName))
    {
      other._connection = nullptr;
    }

    DataFactory& operator=(DataFactory&& other) noexcept
    {
      if (this != &other)
      {
        if (_connection != nullptr) _connection->releaseInstance();
        _connection = other._connection;
        _tableName = std::move(other._tableName);
        other._connection = nullptr;
      }
      return *this;
    }

    void executeModificationSql(const std::string& query)
    {
      requireConnection();
      std::lock_guard<std::mutex> guard(_connection->getQueryMutex());

      if (mysql_real_query(_connection->getConnection(), query.c_str(),
                            static_cast<unsigned long>(query.size())) != 0)
      {
        processError("executeModificationSql: " + query);
        return;
      }

      if (mysql_affected_rows(_connection->getConnection()) == 0)
      {
        throw EntryNotFound();
      }
    }

    /// Caller owns the result (mysql_free_result()) -- runQuery() (Data.h)
    /// does that for you.
    MYSQL_RES* executeQuery(const std::string& query)
    {
      requireConnection();
      std::lock_guard<std::mutex> guard(_connection->getQueryMutex());

      if (mysql_real_query(_connection->getConnection(), query.c_str(),
                            static_cast<unsigned long>(query.size())) != 0)
      {
        processError("executeQuery: " + query);
      }

      MYSQL_RES* result = mysql_store_result(_connection->getConnection());
      if (result == nullptr && mysql_errno(_connection->getConnection()) != 0)
      {
        processError("mysql_store_result: " + query);
      }
      return result;
    }

    void processError(const std::string& errorLoc = "", bool throwExceptions = true)
    {
      if (_connection == nullptr)
      {
        if (throwExceptions) throw ServiceUnavailable("no connection");
        return;
      }
      _connection->processError(errorLoc, throwExceptions, _tableName);
    }

    const std::string& getTableName() const { return _tableName; }

    static std::string getNullableStringColumnValue(const std::string& stringValue)
    {
      if (stringValue.empty()) return "null";
      std::string escaped;
      escaped.reserve(stringValue.size() + 2);
      escaped += '\'';
      for (char c : stringValue)
      {
        if (c == '\'' || c == '\\') escaped += '\\';
        escaped += c;
      }
      escaped += '\'';
      return escaped;
    }

    static std::string getNullableIntegerColumnValue(int value)
    {
      if (value == 0) return "null";
      return std::to_string(value);
    }

    /// A genuine SQL NULL cell (nullptr) maps to std::nullopt, distinct
    /// from a real empty string.
    static std::optional<std::string> fetchNullableStringColumnValue(const char* cell)
    {
      if (cell == nullptr) return std::nullopt;
      return std::string(cell);
    }

    static std::optional<int> fetchNullableIntegerColumnValue(const char* cell)
    {
      if (cell == nullptr) return std::nullopt;
      return std::atoi(cell);
    }

    static constexpr std::size_t maxCharLength = 33;
    static constexpr std::size_t maxLongCharLength = 256;

  private:
    void requireConnection()
    {
      if (_connection == nullptr) throw ServiceUnavailable("DataFactory not connected");
    }

    static bool& haveDefaultParams() { static bool b = false; return b; }
    static MySqlConnectionParams& defaultParams() { static MySqlConnectionParams p; return p; }
    static std::mutex& defaultParamsMutex() { static std::mutex m; return m; }

    MySqlConnection* _connection = nullptr;
    std::string _tableName;
  };

  /// Converts one fetched MYSQL_ROW into an Entry -- what a concrete
  /// table implements per-schema. See WidgetTable.h.
  template<class Entry>
  class RowMapper
  {
  public:
    virtual ~RowMapper() = default;
    virtual Entry mapRow(MYSQL_ROW row, unsigned long* lengths) const = 0;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_DATAFACTORY_H
