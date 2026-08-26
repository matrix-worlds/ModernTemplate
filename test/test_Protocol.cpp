/* -*- C++ -*- */
//
// test_Protocol.cpp
//
// Exercises ProtocolStateMachine<S,MessageBase> directly, with small
// throwaway state/message types -- see Protocol.h for the design
// rationale (a real, inspectable (state, message-type) -> handler
// table).

#include "MiniTest.h"
#include "Message.h"
#include "Protocol.h"
#include "State.h"
#include <vector>

using namespace ModernCommon;

namespace {

  // A tiny two-state, two-message-type protocol used only by this test.

  class OpenState : public State
  {
  public:
    static const OpenState& instance() { static const OpenState s; return s; }
  private:
    OpenState() : State("Open") {}
  };

  class ClosedState : public State
  {
  public:
    static const ClosedState& instance() { static const ClosedState s; return s; }
  private:
    ClosedState() : State("Closed") {}
  };

  class CloseMessage : public Message
  {
  public:
    std::string getName() const override { return "Close"; }
  };

  class OpenMessage : public Message
  {
  public:
    std::string getName() const override { return "Open"; }
  };

  using TestProtocol = ProtocolStateMachine<State, Message>;

  void test_transitionTableStartsEmpty(MiniTest& t)
  {
    TestProtocol p("p1");
    MT_CHECK(t, p.getTransitionTableSize() == 0);
  }

  void test_onMessageRegistersOneEntry(MiniTest& t)
  {
    TestProtocol p("p2");
    p.onMessage<CloseMessage>(&OpenState::instance(),
      [](TestProtocol&, const CloseMessage&) {});
    MT_CHECK(t, p.getTransitionTableSize() == 1);
  }

  void test_receiveDispatchesToTheRegisteredHandler(MiniTest& t)
  {
    TestProtocol p("p3");
    bool handlerRan = false;

    p.onMessage<CloseMessage>(&OpenState::instance(),
      [&](TestProtocol& proto, const CloseMessage&) {
        handlerRan = true;
        proto.setCurrentState(&ClosedState::instance());
      });

    p.setCurrentState(&OpenState::instance());
    p.receive(CloseMessage());

    MT_CHECK(t, handlerRan);
    MT_CHECK(t, p.getCurrentState() == &ClosedState::instance());
  }

  /// The same message type in a *different* state is a different table
  /// entry -- dispatch is keyed on (state, message-type), not message-type
  /// alone.
  void test_dispatchIsStateSpecific(MiniTest& t)
  {
    TestProtocol p("p4");
    std::vector<std::string> ran;

    p.onMessage<CloseMessage>(&OpenState::instance(),
      [&](TestProtocol&, const CloseMessage&) { ran.push_back("open-close"); });
    p.onMessage<CloseMessage>(&ClosedState::instance(),
      [&](TestProtocol&, const CloseMessage&) { ran.push_back("closed-close"); });

    p.setCurrentState(&OpenState::instance());
    p.receive(CloseMessage());

    p.setCurrentState(&ClosedState::instance());
    p.receive(CloseMessage());

    MT_CHECK(t, ran.size() == 2);
    MT_CHECK(t, ran[0] == "open-close");
    MT_CHECK(t, ran[1] == "closed-close");
  }

  /// Dispatch is keyed on the message's *runtime* type, matched via
  /// typeid -- registering a handler for CloseMessage must not fire for
  /// an unrelated OpenMessage received in the same state.
  void test_dispatchIsMessageTypeSpecific(MiniTest& t)
  {
    TestProtocol p("p5");
    int closeCount = 0;

    p.onMessage<CloseMessage>(&OpenState::instance(),
      [&](TestProtocol&, const CloseMessage&) { ++closeCount; });

    p.setCurrentState(&OpenState::instance());
    p.receive(OpenMessage()); // no handler registered for this combo

    MT_CHECK(t, closeCount == 0);
  }

  void test_unhandledFallbackFiresWhenNoEntryMatches(MiniTest& t)
  {
    TestProtocol p("p6");
    bool unhandledRan = false;
    std::string unhandledMsgName;

    p.onUnhandled([&](TestProtocol&, const Message& msg) {
      unhandledRan = true;
      unhandledMsgName = msg.getName();
    });

    p.setCurrentState(&OpenState::instance());
    p.receive(OpenMessage()); // nothing registered for (Open, OpenMessage)

    MT_CHECK(t, unhandledRan);
    MT_CHECK(t, unhandledMsgName == "Open");
  }

  void test_noUnhandledHookMeansSilentNoOp(MiniTest& t)
  {
    TestProtocol p("p7"); // no onUnhandled() call at all
    p.setCurrentState(&OpenState::instance());
    MT_NO_THROW(t, p.receive(OpenMessage()));
  }

} // namespace

int main()
{
  MiniTest t("test_Protocol");
  MT_RUN(t, test_transitionTableStartsEmpty);
  MT_RUN(t, test_onMessageRegistersOneEntry);
  MT_RUN(t, test_receiveDispatchesToTheRegisteredHandler);
  MT_RUN(t, test_dispatchIsStateSpecific);
  MT_RUN(t, test_dispatchIsMessageTypeSpecific);
  MT_RUN(t, test_unhandledFallbackFiresWhenNoEntryMatches);
  MT_RUN(t, test_noUnhandledHookMeansSilentNoOp);
  return t.result();
}
