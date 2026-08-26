/* -*- C++ -*- */
//
// test_State.cpp
//
// Exercises State -- see State.h for the design rationale (protected
// constructor, no operator==/!=, singleton-state recommendation).

#include "MiniTest.h"
#include "State.h"
#include "StateMachine.h"

using namespace ModernCommon;

namespace {

  /// A minimal concrete state that records whether/how its hooks fired,
  /// for test assertions.
  class RecordingState : public State
  {
  public:
    explicit RecordingState(std::string name) : State(std::move(name)) {}

    void onEntry(StateMachineBase& sm, const State* oldState) const override
    {
      ++entryCount;
      lastEntrySourceName = (oldState != nullptr) ? oldState->getName() : "";
      lastEntryMachineName = sm.getName();
    }

    void onExit(StateMachineBase& sm, const State* newState) const override
    {
      ++exitCount;
      lastExitDestName = (newState != nullptr) ? newState->getName() : "";
    }

    mutable int entryCount = 0;
    mutable int exitCount = 0;
    mutable std::string lastEntrySourceName;
    mutable std::string lastEntryMachineName;
    mutable std::string lastExitDestName;
  };

  void test_getName(MiniTest& t)
  {
    RecordingState s("Alpha");
    MT_CHECK(t, s.getName() == "Alpha");
  }

  void test_defaultHooksAreNoOpsOnPlainState(MiniTest& t)
  {
    // A State constructed without overriding onEntry/onExit (impossible
    // to instantiate State directly -- constructor is protected -- so
    // this uses the smallest possible subclass that changes nothing).
    class PlainState : public State
    {
    public:
      PlainState() : State("Plain") {}
    };

    PlainState s;
    StateMachine<PlainState> sm("sm");
    MT_NO_THROW(t, sm.setCurrentState(&s)); // default onEntry/onExit: no-ops, no throw
    MT_CHECK(t, sm.getCurrentState() == &s);
  }

  void test_identityIsPointerEquality(MiniTest& t)
  {
    // No operator==/!= on State (see file comment) -- identity is
    // whether two pointers/references name the same object, which is
    // exactly what a singleton state's instance() gives you for free.
    RecordingState a("A");
    RecordingState b("A"); // same name, different object
    MT_CHECK(t, &a != &b);
    MT_CHECK(t, &a == &a);
  }

  void test_singletonPatternYieldsOneSharedInstance(MiniTest& t)
  {
    class SingletonState : public State
    {
    public:
      static const SingletonState& instance()
      {
        static const SingletonState s;
        return s;
      }

    private:
      SingletonState() : State("TheOneState") {}
    };

    const SingletonState& a = SingletonState::instance();
    const SingletonState& b = SingletonState::instance();
    MT_CHECK(t, &a == &b);
  }

} // namespace

int main()
{
  MiniTest t("test_State");
  MT_RUN(t, test_getName);
  MT_RUN(t, test_defaultHooksAreNoOpsOnPlainState);
  MT_RUN(t, test_identityIsPointerEquality);
  MT_RUN(t, test_singletonPatternYieldsOneSharedInstance);
  return t.result();
}
