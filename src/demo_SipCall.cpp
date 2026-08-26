/* -*- C++ -*- */
//
// demo_SipCall.cpp
//
// A runnable, console demonstration of InviteClientTransaction driving
// three realistic call scenarios (answered, rejected, no answer) end to
// end. Not a network client -- no wire encoding, no sockets -- see
// SipTransaction.h's file comment for what's in and out of scope. Every
// action a real transport/TU layer would have received is printed here,
// so the RFC 3261 SS17.1.1 message flow (Figure 5) is visible directly
// rather than only asserted by a test.
//
// Run: ./demo_SipCall

#include "SipTransaction.h"
#include <iostream>
#include <string>

using namespace ModernCommon;

namespace {

  void printHeader(const std::string& title)
  {
    std::cout << "\n=== " << title << " ===\n";
  }

  InviteClientTransaction::SendRequest logSendRequest()
  {
    return [](const SipRequest& r) {
      std::cout << "  --> " << r.getName() << " " << r.getRequestUri() << "\n";
    };
  }

  InviteClientTransaction::SendAck logSendAck()
  {
    return [](const SipResponse& r) {
      std::cout << "  --> ACK (for " << r.getName() << ")\n";
    };
  }

  InviteClientTransaction::NotifyTU logNotifyTU()
  {
    return [](const SipResponse& r) {
      std::cout << "  [TU notified] " << r.getName() << "\n";
    };
  }

  InviteClientTransaction::NotifyTUError logNotifyTUError()
  {
    return [](const std::string& reason) {
      std::cout << "  [TU notified: FAILURE] " << reason << "\n";
    };
  }

  InviteRequest makeInvite()
  {
    return InviteRequest("sip:bob@example.com", "call-demo-1", "z9hG4bK-demo", 1);
  }

  void printState(const InviteClientTransaction& txn)
  {
    const State* s = txn.getCurrentState();
    std::cout << "  [state: " << (s != nullptr ? s->getName() : "UNDEFINED_STATE") << "]\n";
  }

  void scenarioAnsweredCall()
  {
    printHeader("Scenario 1: call answered");
    InviteClientTransaction txn("demo-answered", makeInvite(), logSendRequest(), logSendAck(),
                                 logNotifyTU(), logNotifyTUError());
    printState(txn);

    std::cout << "  <-- 180 Ringing\n";
    txn.receive(ProvisionalResponse(180, "Ringing", "call-demo-1", "z9hG4bK-demo", 1, SipMethod::Invite));
    printState(txn);

    std::cout << "  <-- 200 OK\n";
    txn.receive(SuccessResponse(200, "OK", "call-demo-1", "z9hG4bK-demo", 1, SipMethod::Invite));
    printState(txn);
  }

  void scenarioRejectedCall()
  {
    printHeader("Scenario 2: call rejected (busy)");
    InviteClientTransaction txn("demo-busy", makeInvite(), logSendRequest(), logSendAck(),
                                 logNotifyTU(), logNotifyTUError());
    printState(txn);

    std::cout << "  <-- 486 Busy Here\n";
    txn.receive(ClientErrorResponse(486, "Busy Here", "call-demo-1", "z9hG4bK-demo", 1, SipMethod::Invite));
    printState(txn);

    std::cout << "  (Timer D fires)\n";
    txn.receive(TransactionWaitTimeoutEvent());
    printState(txn);
  }

  void scenarioNoAnswerTimeout()
  {
    printHeader("Scenario 3: no response (Timer B timeout)");
    InviteClientTransaction txn("demo-timeout", makeInvite(), logSendRequest(), logSendAck(),
                                 logNotifyTU(), logNotifyTUError());
    printState(txn);

    std::cout << "  (Timer B fires -- no response ever arrived)\n";
    txn.receive(TransactionTimeoutEvent());
    printState(txn);
  }

} // namespace

int main()
{
  scenarioAnsweredCall();
  scenarioRejectedCall();
  scenarioNoAnswerTimeout();
  std::cout << "\n";
  return 0;
}
