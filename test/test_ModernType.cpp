/* -*- C++ -*- */
//
// test_ModernType.cpp
//
// Exercises ModernType<Type> -- see ModernType.h for the design
// rationale (defaulted operator<=> as a hidden friend, no Stringable
// base).

#include "MiniTest.h"
#include "ModernType.h"
#include <compare>

using namespace ModernCommon;

namespace {

  using IntType = ModernType<int>;

  void test_defaultConstruction(MiniTest& t)
  {
    IntType v;
    MT_CHECK(t, v.getValue() == 0);
  }

  void test_explicitConstruction(MiniTest& t)
  {
    IntType v(42);
    MT_CHECK(t, v.getValue() == 42);
  }

  void test_setValue(MiniTest& t)
  {
    IntType v;
    v.setValue(7);
    MT_CHECK(t, v.getValue() == 7);
  }

  void test_getString(MiniTest& t)
  {
    IntType v(99);
    MT_CHECK(t, v.getString() == "99");
  }

  void test_equality(MiniTest& t)
  {
    IntType a(5), b(5), c(6);
    MT_CHECK(t, a == b);
    MT_CHECK(t, !(a == c));
  }

  void test_ordering(MiniTest& t)
  {
    IntType a(1), b(2);
    MT_CHECK(t, a < b);
    MT_CHECK(t, b > a);
    MT_CHECK(t, a <= a);
    MT_CHECK(t, a >= a);
    MT_CHECK(t, (a <=> b) == std::strong_ordering::less);
  }

  void test_copyAndMove(MiniTest& t)
  {
    IntType a(3);
    IntType b = a; // copy
    MT_CHECK(t, b.getValue() == 3);
    IntType c = std::move(a); // move
    MT_CHECK(t, c.getValue() == 3);
  }

  void test_stringType(MiniTest& t)
  {
    ModernType<std::string> v(std::string("hello"));
    MT_CHECK(t, v.getValue() == "hello");
    MT_CHECK(t, v.getString() == "hello");
  }

} // namespace

int main()
{
  MiniTest t("test_ModernType");
  MT_RUN(t, test_defaultConstruction);
  MT_RUN(t, test_explicitConstruction);
  MT_RUN(t, test_setValue);
  MT_RUN(t, test_getString);
  MT_RUN(t, test_equality);
  MT_RUN(t, test_ordering);
  MT_RUN(t, test_copyAndMove);
  MT_RUN(t, test_stringType);
  return t.result();
}
