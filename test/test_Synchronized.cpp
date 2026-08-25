/* -*- C++ -*- */
//
// test_Synchronized.cpp
//
// Exercises Synchronized<T> and SynchronizedMap<Key,Data> -- see
// Synchronized.h for the design rationale.

#include "MiniTest.h"
#include "Synchronized.h"
#include <string>
#include <thread>
#include <vector>

using namespace ModernCommon;

namespace {

  void test_synchronizedBasic(MiniTest& t)
  {
    Synchronized<int> v(5);
    int got = v.withLock([](int& x) { return x; });
    MT_CHECK(t, got == 5);

    v.withLock([](int& x) { x += 10; });
    MT_CHECK(t, v.withLock([](int& x) { return x; }) == 15);
  }

  void test_synchronizedConcurrentIncrements(MiniTest& t)
  {
    Synchronized<int> counter(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
    {
      threads.emplace_back([&counter]() {
        for (int j = 0; j < 1000; ++j)
        {
          counter.withLock([](int& x) { ++x; });
        }
      });
    }
    for (auto& th : threads) th.join();
    MT_CHECK(t, counter.withLock([](int& x) { return x; }) == 8000);
  }

  void test_mapAddAndLookup(MiniTest& t)
  {
    SynchronizedMap<int, std::string> m;
    MT_CHECK(t, m.addDataEntry(1, "one"));
    MT_CHECK(t, !m.addDataEntry(1, "uno")); // already present
    auto found = m.lookupDataEntry(1);
    MT_CHECK(t, found.has_value());
    MT_CHECK(t, *found == "one");
    MT_CHECK(t, !m.lookupDataEntry(2).has_value());
  }

  void test_mapRemove(MiniTest& t)
  {
    SynchronizedMap<int, std::string> m;
    m.addDataEntry(1, "one");
    m.removeDataEntry(1);
    MT_CHECK(t, !m.lookupDataEntry(1).has_value());
  }

  void test_mapSize(MiniTest& t)
  {
    SynchronizedMap<int, std::string> m;
    MT_CHECK(t, m.size() == 0);
    m.addDataEntry(1, "one");
    m.addDataEntry(2, "two");
    MT_CHECK(t, m.size() == 2);
  }

  void test_mapReload(MiniTest& t)
  {
    SynchronizedMap<int, std::string> a;
    a.addDataEntry(1, "one");
    a.addDataEntry(2, "two");

    SynchronizedMap<int, std::string> b;
    b.addDataEntry(99, "stale");

    b.reload(a);
    MT_CHECK(t, b.size() == 2);
    MT_CHECK(t, !b.lookupDataEntry(99).has_value());
    MT_CHECK(t, b.lookupDataEntry(1).value() == "one");
  }

  void test_mapSnapshot(MiniTest& t)
  {
    SynchronizedMap<int, std::string> m;
    m.addDataEntry(1, "one");
    m.addDataEntry(2, "two");
    auto snap = m.snapshot();
    MT_CHECK(t, snap.size() == 2);
  }

} // namespace

int main()
{
  MiniTest t("test_Synchronized");
  MT_RUN(t, test_synchronizedBasic);
  MT_RUN(t, test_synchronizedConcurrentIncrements);
  MT_RUN(t, test_mapAddAndLookup);
  MT_RUN(t, test_mapRemove);
  MT_RUN(t, test_mapSize);
  MT_RUN(t, test_mapReload);
  MT_RUN(t, test_mapSnapshot);
  return t.result();
}
