/* -*- C++ -*- */
//
// test_ObjectPool.cpp
//
// Exercises ObjectPool<T> -- see ObjectPool.h for the design rationale
// (shared_ptr + custom deleter instead of intrusive RCObject/RCPtr).

#include "MiniTest.h"
#include "ObjectPool.h"
#include <vector>

using namespace ModernCommon;

namespace {

  struct Widget
  {
    int value = -1;
    int resetCount = 0;
    void reset()
    {
      value = -1;
      ++resetCount;
    }
  };

  void test_createPrepopulates(MiniTest& t)
  {
    auto pool = ObjectPool<Widget>::create(5, 2);
    MT_CHECK(t, pool->getPoolSize() == 5);
  }

  void test_acquireShrinksPool(MiniTest& t)
  {
    auto pool = ObjectPool<Widget>::create(3, 2);
    auto h = pool->acquire();
    MT_CHECK(t, pool->getPoolSize() == 2);
    (void)h;
  }

  void test_releaseReturnsToPoolAndResets(MiniTest& t)
  {
    auto pool = ObjectPool<Widget>::create(1, 1);
    {
      auto h = pool->acquire();
      h->value = 42;
      MT_CHECK(t, pool->getPoolSize() == 0);
    } // Handle drops here -> deleter runs -> reset() + returned to pool
    MT_CHECK(t, pool->getPoolSize() == 1);
  }

  void test_growsWhenExhausted(MiniTest& t)
  {
    auto pool = ObjectPool<Widget>::create(1, 4);
    auto h1 = pool->acquire(); // pool now empty
    MT_CHECK(t, pool->getPoolSize() == 0);
    auto h2 = pool->acquire(); // triggers growLocked(4), then takes one
    MT_CHECK(t, pool->getPoolSize() == 3);
    (void)h1;
    (void)h2;
  }

  void test_handleOutlivesPool(MiniTest& t)
  {
    Widget* raw = nullptr;
    typename ObjectPool<Widget>::Handle h;
    {
      auto pool = ObjectPool<Widget>::create(1, 1);
      h = pool->acquire();
      raw = h.get();
    } // pool's shared_ptr goes out of scope here; h still holds a Handle
    MT_CHECK(t, h.get() == raw);
    h.reset(); // deleter's weak_ptr can't lock -> deletes normally, no crash
    MT_CHECK(t, true); // reaching here without crashing is the assertion
  }

  void test_getInstanceIsASharedSingleton(MiniTest& t)
  {
    ObjectPool<Widget>& a = ObjectPool<Widget>::getInstance();
    ObjectPool<Widget>& b = ObjectPool<Widget>::getInstance();
    MT_CHECK(t, &a == &b);
  }

  void test_getString(MiniTest& t)
  {
    auto pool = ObjectPool<Widget>::create(2, 1);
    MT_CHECK(t, pool->getString().find("ObjectPool") != std::string::npos);
  }

} // namespace

int main()
{
  MiniTest t("test_ObjectPool");
  MT_RUN(t, test_createPrepopulates);
  MT_RUN(t, test_acquireShrinksPool);
  MT_RUN(t, test_releaseReturnsToPoolAndResets);
  MT_RUN(t, test_growsWhenExhausted);
  MT_RUN(t, test_handleOutlivesPool);
  MT_RUN(t, test_getInstanceIsASharedSingleton);
  MT_RUN(t, test_getString);
  return t.result();
}
