/* -*- C++ -*- */
//
// test_SipTransaction.cpp
//
// Exercises InviteClientTransaction against RFC 3261 Figure 5's shape --
// see SipTransaction.h for the design rationale and what's out of scope
// (Timer A / retransmission scheduling).

#include "MiniTest.h"
#include "SipTransaction.h"
#include <vector>

using namespace ModernCommon;

namespace {

  /// Records every callback invocation for assertion, standing in for
  /// the transport layer and the transaction user (TU).
  struct Harness
  {
    std::vector<std::string> sentRequests;
    std::vector<std::string> sentAcks;
    std::vector<std::string> tuNotifications;
    std::vector<std::string> tuErrors;

    InviteClientTransaction::SendRequest sendRequest()
    {
      return [this](const SipRequest& r) { sentRequests.push_back(r.getName()); };
    }
    InviteClientTransaction::SendAck sendAck()
    {
      return [this](const SipResponse& r) { sentAcks.push_back(r.getName()); };
    }
    InviteClientTransaction::NotifyTU notifyTU()
    {
      return [this](const SipResponse& r) { tuNotifications.push_back(r.getName()); };
    }
    InviteClientTransaction::NotifyTUError notifyTUError()
    {
      return [this](const std::string& reason) { tuErrors.push_back(reason); };
    }
  };

  InviteRequest makeInvite() { return InviteRequest("sip:bob@example.com", "call-1", "z9hG4bK-1", 1); }

  void test_constructionSendsInviteAndEntersCalling(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn1", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    MT_CHECK(t, txn.getCurrentState() == &InviteClientCallingState::instance());
    MT_CHECK(t, h.sentRequests.size() == 1);
    MT_CHECK(t, h.sentRequests[0] == "INVITE");
  }

  void test_provisionalMovesCallingToProceeding(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn2", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(ProvisionalResponse(180, "Ringing", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientProceedingState::instance());
    MT_CHECK(t, h.tuNotifications.size() == 1);
    MT_CHECK(t, h.tuNotifications[0] == "180 Ringing");
  }

  void test_secondProvisionalStaysInProceeding(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn3", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(ProvisionalResponse(180, "Ringing", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));
    txn.receive(ProvisionalResponse(183, "Session Progress", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientProceedingState::instance());
    MT_CHECK(t, h.tuNotifications.size() == 2);
  }

  void test_successFromCallingTerminatesWithoutAck(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn4", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(SuccessResponse(200, "OK", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientTerminatedState::instance());
    MT_CHECK(t, h.tuNotifications.size() == 1);
    MT_CHECK(t, h.sentAcks.empty()); // 2xx ACK is the TU/dialog's job, not this transaction's
  }

  void test_successFromProceedingTerminates(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn5", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(ProvisionalResponse(180, "Ringing", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));
    txn.receive(SuccessResponse(200, "OK", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientTerminatedState::instance());
    MT_CHECK(t, h.sentAcks.empty());
  }

  void test_nonSuccessFinalFromCallingSendsAckAndCompletes(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn6", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(ClientErrorResponse(486, "Busy Here", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientCompletedState::instance());
    MT_CHECK(t, h.sentAcks.size() == 1);
    MT_CHECK(t, h.sentAcks[0] == "486 Busy Here");
    MT_CHECK(t, h.tuNotifications.size() == 1);
  }

  void test_nonSuccessFinalFromProceedingSendsAckAndCompletes(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn7", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(ProvisionalResponse(180, "Ringing", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));
    txn.receive(ServerErrorResponse(500, "Server Error", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientCompletedState::instance());
    MT_CHECK(t, h.sentAcks.size() == 1);
  }

  /// Completed absorbs a retransmitted final response by resending the
  /// ACK, without leaving Completed.
  void test_completedAbsorbsRetransmittedFinalResponse(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn8", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(ClientErrorResponse(486, "Busy Here", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));
    txn.receive(ClientErrorResponse(486, "Busy Here", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));
    txn.receive(ClientErrorResponse(486, "Busy Here", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientCompletedState::instance());
    MT_CHECK(t, h.sentAcks.size() == 3); // ACK resent for every retransmission
  }

  void test_timerDFromCompletedTerminates(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn9", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(ClientErrorResponse(486, "Busy Here", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));
    txn.receive(TransactionWaitTimeoutEvent());

    MT_CHECK(t, txn.getCurrentState() == &InviteClientTerminatedState::instance());
  }

  void test_timerBFromCallingTerminatesAndNotifiesError(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn10", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(TransactionTimeoutEvent());

    MT_CHECK(t, txn.getCurrentState() == &InviteClientTerminatedState::instance());
    MT_CHECK(t, h.tuErrors.size() == 1);
  }

  void test_transportErrorFromCallingTerminatesAndNotifiesError(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn11", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(TransportErrorEvent("connection refused"));

    MT_CHECK(t, txn.getCurrentState() == &InviteClientTerminatedState::instance());
    MT_CHECK(t, h.tuErrors.size() == 1);
    MT_CHECK(t, h.tuErrors[0] == "connection refused");
  }

  /// A message arriving after Terminated (e.g. a very late retransmission
  /// after Timer D already fired) is silently discarded -- no crash, no
  /// callback -- per the file comment's onUnhandled() design decision.
  void test_messageAfterTerminatedIsSilentlyDiscarded(MiniTest& t)
  {
    Harness h;
    InviteClientTransaction txn("txn12", makeInvite(), h.sendRequest(), h.sendAck(),
                                 h.notifyTU(), h.notifyTUError());

    txn.receive(SuccessResponse(200, "OK", "call-1", "z9hG4bK-1", 1, SipMethod::Invite));
    MT_CHECK(t, txn.getCurrentState() == &InviteClientTerminatedState::instance());

    MT_NO_THROW(t, txn.receive(ClientErrorResponse(486, "Busy Here", "call-1", "z9hG4bK-1", 1, SipMethod::Invite)));
    MT_CHECK(t, h.sentAcks.empty());
    MT_CHECK(t, h.tuNotifications.size() == 1); // only the original 200, nothing further
  }

} // namespace

int main()
{
  MiniTest t("test_SipTransaction");
  MT_RUN(t, test_constructionSendsInviteAndEntersCalling);
  MT_RUN(t, test_provisionalMovesCallingToProceeding);
  MT_RUN(t, test_secondProvisionalStaysInProceeding);
  MT_RUN(t, test_successFromCallingTerminatesWithoutAck);
  MT_RUN(t, test_successFromProceedingTerminates);
  MT_RUN(t, test_nonSuccessFinalFromCallingSendsAckAndCompletes);
  MT_RUN(t, test_nonSuccessFinalFromProceedingSendsAckAndCompletes);
  MT_RUN(t, test_completedAbsorbsRetransmittedFinalResponse);
  MT_RUN(t, test_timerDFromCompletedTerminates);
  MT_RUN(t, test_timerBFromCallingTerminatesAndNotifiesError);
  MT_RUN(t, test_transportErrorFromCallingTerminatesAndNotifiesError);
  MT_RUN(t, test_messageAfterTerminatedIsSilentlyDiscarded);
  return t.result();
}
