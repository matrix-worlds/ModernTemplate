/* -*- C++ -*- */
//
// MySqlConnection.cpp
//
// See MySqlConnection.h.

#include "MySqlConnection.h"
#include "DataError.h"
#include <sstream>

#include <mysqld_error.h> // ER_* server error codes (ER_DUP_ENTRY etc.)

namespace ModernCommon {

MySqlConnection* MySqlConnection::_instance = nullptr;
std::mutex MySqlConnection::_instanceMutex;

MySqlConnection::MySqlConnection(const MySqlConnectionParams& params)
  : _mysql(nullptr)
{
  _mysql = mysql_init(nullptr);
  if (_mysql == nullptr)
  {
    throw ServiceUnavailable("mysql_init() failed");
  }

  if (mysql_real_connect(_mysql,
                          params.host.c_str(),
                          params.user.c_str(),
                          params.password.c_str(),
                          params.database.c_str(),
                          params.port,
                          nullptr,
                          0) == nullptr)
  {
    std::ostringstream ost;
    ost << "mysql_real_connect() to " << params.host << ":" << params.port
        << " failed: " << mysql_error(_mysql);
    mysql_close(_mysql);
    _mysql = nullptr;
    throw ServiceUnavailable(ost.str());
  }

  mysql_autocommit(_mysql, 1);
}

MySqlConnection::~MySqlConnection()
{
  if (_mysql != nullptr) mysql_close(_mysql);
}

MySqlConnection*
MySqlConnection::getCurrentInstance(const MySqlConnectionParams& params)
{
  std::lock_guard<std::mutex> guard(_instanceMutex);

  if (_instance == nullptr)
  {
    _instance = new MySqlConnection(params);
  }
  ++_instance->_count;
  return _instance;
}

void
MySqlConnection::disconnect()
{
  std::lock_guard<std::mutex> guard(_instanceMutex);
  if (_instance != nullptr)
  {
    _instance->_destroyWhenIdle = true;
    if (_instance->_count == 0)
    {
      delete _instance;
      _instance = nullptr;
    }
  }
}

void
MySqlConnection::releaseInstance()
{
  std::lock_guard<std::mutex> guard(_instanceMutex);
  --_count;
  if (_count == 0 && _destroyWhenIdle && this == _instance)
  {
    _instance = nullptr;
    delete this;
  }
}

void
MySqlConnection::startTransaction()
{
  if (_mysql == nullptr) throw ServiceUnavailable();
  mysql_autocommit(_mysql, 0);
  _transactionActive = true;
}

void
MySqlConnection::commitTransaction()
{
  if (!_transactionActive) return;
  if (_mysql != nullptr) mysql_commit(_mysql);
  _transactionActive = false;
  if (_mysql != nullptr) mysql_autocommit(_mysql, 1);
}

void
MySqlConnection::rollbackTransaction()
{
  if (!_transactionActive) return;
  if (_mysql != nullptr) mysql_rollback(_mysql);
  _transactionActive = false;
  if (_mysql != nullptr) mysql_autocommit(_mysql, 1);
}

void
MySqlConnection::processError(const std::string& errorLoc,
                               bool throwExceptions,
                               const std::string& tableName)
{
  if (_mysql == nullptr)
  {
    if (throwExceptions) throw ServiceUnavailable("no connection");
    return;
  }

  unsigned int errNo = mysql_errno(_mysql);
  if (errNo == 0) return;

  std::ostringstream ost;
  ost << (tableName.empty() ? std::string() : tableName + ": ")
      << errorLoc << ": " << mysql_error(_mysql) << " (errno " << errNo << ")";

  if (!throwExceptions) return;

  switch (errNo)
  {
    case ER_DUP_ENTRY:
      throw DuplicateEntry(ost.str());

    case ER_ROW_IS_REFERENCED:
    case ER_ROW_IS_REFERENCED_2:
    case ER_NO_REFERENCED_ROW:
    case ER_NO_REFERENCED_ROW_2:
      throw DependencyViolation(ost.str());

    case CR_CONNECTION_ERROR:
    case CR_CONN_HOST_ERROR:
    case CR_SERVER_GONE_ERROR:
    case CR_SERVER_LOST:
      throw ServiceUnavailable(ost.str());

    default:
      throw ServiceUnavailable(ost.str());
  }
}

} // namespace ModernCommon
