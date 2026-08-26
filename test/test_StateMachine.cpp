/* -*- C++ -*- */
//
// test_StateMachine.cpp
//
// Exercises StateMachine<S> -- see StateMachine.h for the design
// rationale (const S* current state, TransitionObserver replacing
// isdx's hardcoded ISDX_LOG transition logging).

#include "MiniTest.h"
#include "State.h"
#include "StateMachine.h"
#include <vector>

using namespace ModernCommon;

namespace {

  class RecordingState : public State
  {
  public:
    explicit RecordingState(std::string name) : State(std::move(name)) {}

    void onEntry(StateMachineBase&, const State* oldState) const override
    {
      entryLog.push_back((oldState != nullptr) ? oldState->getName() : "<none>");
    }

    void onExit(StateMachineBase&, const State* newState) const override
    {
      exitLog.push_back((newState != nullptr) ? newState->getName() : "<none>");
    }

    mutable std::vector<std::string> entryLog;
    mutable std::vector<std::string> exitLog;
  };

  void test_initialStateIsUndefined(MiniTest& t)
  {
    StateMachine<RecordingState> sm("sm1");
    MT_CHECK(t, sm.getCurrentState() == nullptr);
  }

  void test_getName(MiniTest& t)
  {
    StateMachine<RecordingState> sm("sm-name");
    MT_CHECK(t, sm.getName() == "sm-name");
  }

  void test_setCurrentStateFromUndefined(MiniTest& t)
  {
    RecordingState a("A");
    StateMachine<RecordingState> sm("sm2");

    sm.setCurrentState(&a);

    MT_CHECK(t, sm.getCurrentState() == &a);
    MT_CHECK(t, a.entryLog.size() == 1);
    MT_CHECK(t, a.entryLog[0] == "<none>"); // no old state
    MT_CHECK(t, a.exitLog.empty());         // never exited
  }

  void test_transitionRunsExitThenEntry(MiniTest& t)
  {
    RecordingState a("A");
    RecordingState b("B");
    StateMachine<RecordingState> sm("sm3");

    sm.setCurrentState(&a);
    sm.setCurrentState(&b);

    MT_CHECK(t, sm.getCurrentState() == &b);
    MT_CHECK(t, a.exitLog.size() == 1);
    MT_CHECK(t, a.exitLog[0] == "B"); // exiting towards B
    MT_CHECK(t, b.entryLog.size() == 1);
    MT_CHECK(t, b.entryLog[0] == "A"); // entering from A
  }

  void test_setCurrentStateToNullptrIsValid(MiniTest& t)
  {
    RecordingState a("A");
    StateMachine<RecordingState> sm("sm4");

    sm.setCurrentState(&a);
    sm.setCurrentState(nullptr);

    MT_CHECK(t, sm.getCurrentState() == nullptr);
    MT_CHECK(t, a.exitLog.size() == 1);
    MT_CHECK(t, a.exitLog[0] == "<none>");
  }

  void test_transitionObserverReceivesNamesInOrder(MiniTest& t)
  {
    RecordingState a("A");
    RecordingState b("B");
    StateMachine<RecordingState> sm("observed-sm");

    std::vector<std::string> observed; // "machine:old->new" per call
    sm.setTransitionObserver([&](const std::string& name,
                                  const std::string& oldName,
                                  const std::string& newName) {
      observed.push_back(name + ":" + oldName + "->" + newName);
    });

    sm.setCurrentState(&a);
    sm.setCurrentState(&b);

    MT_CHECK(t, observed.size() == 2);
    MT_CHECK(t, observed[0] == "observed-sm:UNDEFINED_STATE->A");
    MT_CHECK(t, observed[1] == "observed-sm:A->B");
  }

  void test_getStringIncludesNameAndCurrentState(MiniTest& t)
  {
    RecordingState a("Alpha");
    StateMachine<RecordingState> sm("sm5");

    MT_CHECK(t, sm.getString().find("UNDEFINED_STATE") != std::string::npos);

    sm.setCurrentState(&a);
    MT_CHECK(t, sm.getString().find("Alpha") != std::string::npos);
    MT_CHECK(t, sm.getString().find("sm5") != std::string::npos);
  }

} // namespace

int main()
{
  MiniTest t("test_StateMachine");
  MT_RUN(t, test_initialStateIsUndefined);
  MT_RUN(t, test_getName);
  MT_RUN(t, test_setCurrentStateFromUndefined);
  MT_RUN(t, test_transitionRunsExitThenEntry);
  MT_RUN(t, test_setCurrentStateToNullptrIsValid);
  MT_RUN(t, test_transitionObserverReceivesNamesInOrder);
  MT_RUN(t, test_getStringIncludesNameAndCurrentState);
  return t.result();
}
