cmake_minimum_required(VERSION 3.16)
project(ModernTemplate CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

# --- MySQL client library (headers + libmysqlclient) -----------------
# Same lookup as MyTemplate's CMakeLists.txt: brew's mysql-client is
# keg-only, so it needs to be found explicitly.
find_path(MYSQL_INCLUDE_DIR
  NAMES mysql.h
  PATH_SUFFIXES mysql
  HINTS /opt/homebrew/opt/mysql-client/include /opt/homebrew/opt/mysql/include
        /usr/local/opt/mysql-client/include /usr/local/opt/mysql/include
  PATHS /usr/include /usr/local/include
)
find_library(MYSQL_LIBRARY
  NAMES mysqlclient
  HINTS /opt/homebrew/opt/mysql-client/lib /opt/homebrew/opt/mysql/lib
        /usr/local/opt/mysql-client/lib /usr/local/opt/mysql/lib
  PATHS /usr/lib /usr/local/lib
)
if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARY)
  message(FATAL_ERROR
    "MySQL client library not found. Install it with "
    "'brew install mysql-client' (macOS) or your distro's libmysqlclient-dev "
    "package, or point MYSQL_INCLUDE_DIR/MYSQL_LIBRARY at it manually.")
endif()
message(STATUS "MySQL include dir: ${MYSQL_INCLUDE_DIR}")
message(STATUS "MySQL library: ${MYSQL_LIBRARY}")

add_library(moderncommon
  src/Dump.cpp
  src/MySqlConnection.cpp
  src/WidgetTable.cpp
)
target_include_directories(moderncommon PUBLIC ${MYSQL_INCLUDE_DIR})
target_link_libraries(moderncommon PUBLIC Threads::Threads ${MYSQL_LIBRARY})

enable_testing()

add_executable(test_ModernType test/test_ModernType.cpp)
target_link_libraries(test_ModernType moderncommon)
add_test(NAME ModernTypeTest COMMAND test_ModernType)

add_executable(test_Synchronized test/test_Synchronized.cpp)
target_link_libraries(test_Synchronized moderncommon)
add_test(NAME SynchronizedTest COMMAND test_Synchronized)

add_executable(test_ObjectPool test/test_ObjectPool.cpp)
target_link_libraries(test_ObjectPool moderncommon)
add_test(NAME ObjectPoolTest COMMAND test_ObjectPool)

add_executable(test_Dump test/test_Dump.cpp)
target_link_libraries(test_Dump moderncommon)
add_test(NAME DumpTest COMMAND test_Dump)

add_executable(test_Data test/test_Data.cpp)
target_link_libraries(test_Data moderncommon)
add_test(NAME DataTest COMMAND test_Data)

add_executable(test_DataFactory test/test_DataFactory.cpp)
target_link_libraries(test_DataFactory moderncommon)
add_test(NAME DataFactoryTest COMMAND test_DataFactory)

add_executable(test_WidgetTable test/test_WidgetTable.cpp)
target_link_libraries(test_WidgetTable moderncommon)
add_test(NAME WidgetTableTest COMMAND test_WidgetTable)

add_executable(test_CacheData test/test_CacheData.cpp)
target_link_libraries(test_CacheData moderncommon)
add_test(NAME CacheDataTest COMMAND test_CacheData)

# Requires a real, reachable MySQL server; skips itself cleanly (exit 0)
# if there isn't one -- same pattern as MyTemplate's *_Integration tests.
add_executable(test_Integration test/test_Integration.cpp)
target_link_libraries(test_Integration moderncommon)
add_test(NAME IntegrationTest COMMAND test_Integration)
