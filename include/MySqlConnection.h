/* -*- C++ -*- */
//
// MySqlConnection.h
//
// A reference-counted singleton connection every DataFactory instance
// shares -- infrastructure, not a "template pattern," so it's carried
// over from MyTemplate essentially unchanged (namespace rename aside):
// talking to libmysqlclient is talking to libmysqlclient regardless of
// which C++ era the surrounding code is written in.

#ifndef MODERNTEMPLATE_MYSQLCONNECTION_H
#define MODERNTEMPLATE_MYSQLCONNECTION_H

#include <cstdint>
#include <mutex>
#include <string>

#include <mysql.h>

namespace ModernCommon {

  struct MySqlConnectionParams
  {
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    unsigned int port = 3306;
  };

  class MySqlConnection
  {
  public:
    static MySqlConnection* getCurrentInstance(const MySqlConnectionParams& params);
    static void disconnect();

    void releaseInstance();

    void startTransaction();
    void commitTransaction();
    void rollbackTransaction();
    bool isTransactionActive() const { return _transactionActive; }

    MYSQL* getConnection() const { return _mysql; }

    void processError(const std::string& errorLoc = "",
                       bool throwExceptions = true,
                       const std::string& tableName = "");

    std::mutex& getQueryMutex() { return _queryMutex; }

  protected:
    explicit MySqlConnection(const MySqlConnectionParams& params);
    ~MySqlConnection();

    MySqlConnection(const MySqlConnection&) = delete;
    MySqlConnection& operator=(const MySqlConnection&) = delete;

  private:
    MYSQL* _mysql;
    std::uint32_t _count = 0;
    bool _destroyWhenIdle = false;
    bool _transactionActive = false;
    std::mutex _queryMutex;

    static MySqlConnection* _instance;
    static std::mutex _instanceMutex;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_MYSQLCONNECTION_H
