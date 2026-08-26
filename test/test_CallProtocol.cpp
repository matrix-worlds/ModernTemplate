/* -*- C++ -*- */
//
// test_CallProtocol.cpp
//
// Exercises CallProtocol -- see CallProtocol.h for the protocol
// definition (Idle/Ringing/Connected, Setup/Answer/Release).

#include "CallProtocol.h"
#include "MiniTest.h"
#include <stdexcept>

using namespace ModernCommon;

namespace {

  void test_startsInIdle(MiniTest& t)
  {
    CallProtocol call("call-1");
    MT_CHECK(t, call.getCurrentState() == &CallIdleState::instance());
  }

  void test_setupMovesIdleToRinging(MiniTest& t)
  {
    CallProtocol call("call-2");
    call.receive(SetupMessage("alice", "bob"));
    MT_CHECK(t, call.getCurrentState() == &CallRingingState::instance());
  }

  void test_answerMovesRingingToConnected(MiniTest& t)
  {
    CallProtocol call("call-3");
    call.receive(SetupMessage("alice", "bob"));
    call.receive(AnswerMessage());
    MT_CHECK(t, call.getCurrentState() == &CallConnectedState::instance());
  }

  void test_releaseFromRingingReturnsToIdle(MiniTest& t)
  {
    // Callee never answers -- caller (or callee) abandons the call while
    // still Ringing.
    CallProtocol call("call-4");
    call.receive(SetupMessage("alice", "bob"));
    call.receive(ReleaseMessage("no-answer"));
    MT_CHECK(t, call.getCurrentState() == &CallIdleState::instance());
  }

  void test_releaseFromConnectedReturnsToIdle(MiniTest& t)
  {
    CallProtocol call("call-5");
    call.receive(SetupMessage("alice", "bob"));
    call.receive(AnswerMessage());
    call.receive(ReleaseMessage("normal"));
    MT_CHECK(t, call.getCurrentState() == &CallIdleState::instance());
  }

  /// A full happy-path call can be placed again after hanging up --
  /// Idle really is a fresh starting point, not a dead end.
  void test_secondCallAfterReleaseWorks(MiniTest& t)
  {
    CallProtocol call("call-6");
    call.receive(SetupMessage("alice", "bob"));
    call.receive(AnswerMessage());
    call.receive(ReleaseMessage());

    call.receive(SetupMessage("carol", "dave"));
    MT_CHECK(t, call.getCurrentState() == &CallRingingState::instance());
  }

  void test_answerWhileIdleIsProtocolViolation(MiniTest& t)
  {
    CallProtocol call("call-7");
    bool threw = false;
    std::string what;
    try
    {
      call.receive(AnswerMessage()); // no Setup yet
    }
    catch (const std::logic_error& e)
    {
      threw = true;
      what = e.what();
    }
    MT_CHECK(t, threw);
    MT_CHECK(t, what.find("Answer") != std::string::npos);
    MT_CHECK(t, what.find("Idle") != std::string::npos);
  }

  void test_secondSetupWhileRingingIsProtocolViolation(MiniTest& t)
  {
    CallProtocol call("call-8");
    call.receive(SetupMessage("alice", "bob"));

    bool threw = false;
    try
    {
      call.receive(SetupMessage("eve", "mallory"));
    }
    catch (const std::logic_error&)
    {
      threw = true;
    }
    MT_CHECK(t, threw);
    // The offending Setup didn't change anything -- still Ringing from
    // the first (valid) Setup.
    MT_CHECK(t, call.getCurrentState() == &CallRingingState::instance());
  }

  void test_messageFieldsAreAccessible(MiniTest& t)
  {
    SetupMessage setup("alice", "bob");
    MT_CHECK(t, setup.getCaller() == "alice");
    MT_CHECK(t, setup.getCallee() == "bob");

    ReleaseMessage release("busy");
    MT_CHECK(t, release.getReason() == "busy");

    ReleaseMessage defaultRelease;
    MT_CHECK(t, defaultRelease.getReason() == "normal");
  }

} // namespace

int main()
{
  MiniTest t("test_CallProtocol");
  MT_RUN(t, test_startsInIdle);
  MT_RUN(t, test_setupMovesIdleToRinging);
  MT_RUN(t, test_answerMovesRingingToConnected);
  MT_RUN(t, test_releaseFromRingingReturnsToIdle);
  MT_RUN(t, test_releaseFromConnectedReturnsToIdle);
  MT_RUN(t, test_secondCallAfterReleaseWorks);
  MT_RUN(t, test_answerWhileIdleIsProtocolViolation);
  MT_RUN(t, test_secondSetupWhileRingingIsProtocolViolation);
  MT_RUN(t, test_messageFieldsAreAccessible);
  return t.result();
}
