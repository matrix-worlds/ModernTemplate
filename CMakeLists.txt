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

# --- Eigen (header-only C++ template linear algebra library) ---------
find_package(Eigen3 REQUIRED NO_MODULE
  HINTS /opt/homebrew/share/eigen3/cmake /opt/homebrew/opt/eigen/share/eigen3/cmake
        /usr/local/share/eigen3/cmake
)
message(STATUS "Eigen3 version: ${Eigen3_VERSION}")

add_library(moderncommon
  src/Dump.cpp
  src/MySqlConnection.cpp
  src/WidgetTable.cpp
  src/CallProtocol.cpp
  src/SipTransaction.cpp
  src/ToyAttention.cpp
)
target_include_directories(moderncommon PUBLIC ${MYSQL_INCLUDE_DIR})
target_link_libraries(moderncommon PUBLIC Threads::Threads ${MYSQL_LIBRARY} Eigen3::Eigen)

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

# --- State / StateMachine / Message / Protocol ------------------------
# No MySQL involved; see doc/state-protocol.md.

add_executable(test_State test/test_State.cpp)
target_link_libraries(test_State moderncommon)
add_test(NAME StateTest COMMAND test_State)

add_executable(test_StateMachine test/test_StateMachine.cpp)
target_link_libraries(test_StateMachine moderncommon)
add_test(NAME StateMachineTest COMMAND test_StateMachine)

add_executable(test_Message test/test_Message.cpp)
target_link_libraries(test_Message moderncommon)
add_test(NAME MessageTest COMMAND test_Message)

add_executable(test_Protocol test/test_Protocol.cpp)
target_link_libraries(test_Protocol moderncommon)
add_test(NAME ProtocolTest COMMAND test_Protocol)

add_executable(test_CallProtocol test/test_CallProtocol.cpp)
target_link_libraries(test_CallProtocol moderncommon)
add_test(NAME CallProtocolTest COMMAND test_CallProtocol)

# --- SIP (RFC 3261) -----------------------------------------------------
# No MySQL involved; see doc/sip.md.

add_executable(test_SipMessage test/test_SipMessage.cpp)
target_link_libraries(test_SipMessage moderncommon)
add_test(NAME SipMessageTest COMMAND test_SipMessage)

add_executable(test_SipTransaction test/test_SipTransaction.cpp)
target_link_libraries(test_SipTransaction moderncommon)
add_test(NAME SipTransactionTest COMMAND test_SipTransaction)

# Runnable console demo (not a test): prints the RFC 3261 SS17.1.1
# message flow for three call scenarios. `./demo_SipCall` to run.
add_executable(demo_SipCall src/demo_SipCall.cpp)
target_link_libraries(demo_SipCall moderncommon)

# --- Eigen / matrix operations ------------------------------------------
# See doc/eigen-matrix.md.

add_executable(test_EigenBasics test/test_EigenBasics.cpp)
target_link_libraries(test_EigenBasics moderncommon)
add_test(NAME EigenBasicsTest COMMAND test_EigenBasics)

add_executable(test_ToyAttention test/test_ToyAttention.cpp)
target_link_libraries(test_ToyAttention moderncommon)
add_test(NAME ToyAttentionTest COMMAND test_ToyAttention)

# Runnable console demo (not a test): prints Q/K/V and the intermediate
# scores/weights/output matrices for a tiny attention example.
# `./demo_Attention` to run.
add_executable(demo_Attention src/demo_Attention.cpp)
target_link_libraries(demo_Attention moderncommon)
