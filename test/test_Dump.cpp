/* -*- C++ -*- */
//
// test_Dump.cpp
//
// Exercises Dumpable/DiagnosticDumpRegistry -- see Dump.h for the design
// rationale (Meyers singleton, shared_mutex read concurrency).

#include "Dump.h"
#include "MiniTest.h"

using namespace ModernCommon;

namespace {

  class FakeDumpable : public Dumpable
  {
  public:
    explicit FakeDumpable(std::string label) : _label(std::move(label)) {}
    void dump() const override { ++dumpCallCount; }
    mutable int dumpCallCount = 0;
    std::string _label;
  };

  void test_instanceIsASingleton(MiniTest& t)
  {
    DiagnosticDumpRegistry& a = DiagnosticDumpRegistry::instance();
    DiagnosticDumpRegistry& b = DiagnosticDumpRegistry::instance();
    MT_CHECK(t, &a == &b);
  }

  void test_registerIncreasesCount(MiniTest& t)
  {
    auto& registry = DiagnosticDumpRegistry::instance();
    std::size_t before = registry.getNumberOfRegisteredObjects();

    FakeDumpable obj("alpha");
    registry.registerObject("alpha", &obj);
    MT_CHECK(t, registry.getNumberOfRegisteredObjects() == before + 1);

    registry.deregisterObject(&obj);
    MT_CHECK(t, registry.getNumberOfRegisteredObjects() == before);
  }

  void test_dumpByNameCallsOnlyMatching(MiniTest& t)
  {
    auto& registry = DiagnosticDumpRegistry::instance();
    FakeDumpable match1("beta");
    FakeDumpable match2("beta");
    FakeDumpable other("gamma");
    registry.registerObject("beta", &match1);
    registry.registerObject("beta", &match2);
    registry.registerObject("gamma", &other);

    registry.dump("beta");
    MT_CHECK(t, match1.dumpCallCount == 1);
    MT_CHECK(t, match2.dumpCallCount == 1);
    MT_CHECK(t, other.dumpCallCount == 0);

    registry.deregisterObject(&match1);
    registry.deregisterObject(&match2);
    registry.deregisterObject(&other);
  }

  void test_deregisterMissingIsNoop(MiniTest& t)
  {
    auto& registry = DiagnosticDumpRegistry::instance();
    std::size_t before = registry.getNumberOfRegisteredObjects();
    FakeDumpable neverRegistered("delta");
    registry.deregisterObject(&neverRegistered); // should not throw or crash
    MT_CHECK(t, registry.getNumberOfRegisteredObjects() == before);
  }

} // namespace

int main()
{
  MiniTest t("test_Dump");
  MT_RUN(t, test_instanceIsASingleton);
  MT_RUN(t, test_registerIncreasesCount);
  MT_RUN(t, test_dumpByNameCallsOnlyMatching);
  MT_RUN(t, test_deregisterMissingIsNoop);
  return t.result();
}
