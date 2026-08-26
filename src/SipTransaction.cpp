/* -*- C++ -*- */
//
// SipTransaction.cpp
//
// See SipTransaction.h. This constructor is the whole transition table
// for RFC 3261 Figure 5, readable top to bottom -- the point of
// Protocol.h's design (see that file's comment).

#include "SipTransaction.h"

namespace ModernCommon {

using Base = ProtocolStateMachine<State, Message>;

InviteClientTransaction::InviteClientTransaction(std::string name,
                                                   const InviteRequest& invite,
                                                   SendRequest sendRequest,
                                                   SendAck sendAck,
                                                   NotifyTU notifyTU,
                                                   NotifyTUError notifyTUError)
  : ProtocolStateMachine<State, Message>(std::move(name))
{
  // -- Calling (RFC 3261 SS17.1.1.2) --

  onMessage<ProvisionalResponse>(&InviteClientCallingState::instance(),
    [notifyTU](Base& p, const ProvisionalResponse& r) {
      p.setCurrentState(&InviteClientProceedingState::instance());
      notifyTU(r);
    });

  onMessage<SuccessResponse>(&InviteClientCallingState::instance(),
    [notifyTU](Base& p, const SuccessResponse& r) {
      p.setCurrentState(&InviteClientTerminatedState::instance());
      notifyTU(r); // 2xx ACK: TU/dialog's job, not this transaction's
    });

  auto handleCallingNonSuccessFinal = [sendAck, notifyTU](Base& p, const SipResponse& r) {
    p.setCurrentState(&InviteClientCompletedState::instance());
    sendAck(r);
    notifyTU(r);
  };
  onMessage<RedirectionResponse>(&InviteClientCallingState::instance(),
    [handleCallingNonSuccessFinal](Base& p, const RedirectionResponse& r) { handleCallingNonSuccessFinal(p, r); });
  onMessage<ClientErrorResponse>(&InviteClientCallingState::instance(),
    [handleCallingNonSuccessFinal](Base& p, const ClientErrorResponse& r) { handleCallingNonSuccessFinal(p, r); });
  onMessage<ServerErrorResponse>(&InviteClientCallingState::instance(),
    [handleCallingNonSuccessFinal](Base& p, const ServerErrorResponse& r) { handleCallingNonSuccessFinal(p, r); });
  onMessage<GlobalFailureResponse>(&InviteClientCallingState::instance(),
    [handleCallingNonSuccessFinal](Base& p, const GlobalFailureResponse& r) { handleCallingNonSuccessFinal(p, r); });

  onMessage<TransactionTimeoutEvent>(&InviteClientCallingState::instance(),
    [notifyTUError](Base& p, const TransactionTimeoutEvent&) {
      p.setCurrentState(&InviteClientTerminatedState::instance());
      notifyTUError("Timer B: no response received");
    });

  onMessage<TransportErrorEvent>(&InviteClientCallingState::instance(),
    [notifyTUError](Base& p, const TransportErrorEvent& e) {
      p.setCurrentState(&InviteClientTerminatedState::instance());
      notifyTUError(e.getReason());
    });

  // -- Proceeding --

  onMessage<ProvisionalResponse>(&InviteClientProceedingState::instance(),
    [notifyTU](Base&, const ProvisionalResponse& r) { notifyTU(r); }); // stays in Proceeding

  onMessage<SuccessResponse>(&InviteClientProceedingState::instance(),
    [notifyTU](Base& p, const SuccessResponse& r) {
      p.setCurrentState(&InviteClientTerminatedState::instance());
      notifyTU(r);
    });

  auto handleProceedingNonSuccessFinal = [sendAck, notifyTU](Base& p, const SipResponse& r) {
    p.setCurrentState(&InviteClientCompletedState::instance());
    sendAck(r);
    notifyTU(r);
  };
  onMessage<RedirectionResponse>(&InviteClientProceedingState::instance(),
    [handleProceedingNonSuccessFinal](Base& p, const RedirectionResponse& r) { handleProceedingNonSuccessFinal(p, r); });
  onMessage<ClientErrorResponse>(&InviteClientProceedingState::instance(),
    [handleProceedingNonSuccessFinal](Base& p, const ClientErrorResponse& r) { handleProceedingNonSuccessFinal(p, r); });
  onMessage<ServerErrorResponse>(&InviteClientProceedingState::instance(),
    [handleProceedingNonSuccessFinal](Base& p, const ServerErrorResponse& r) { handleProceedingNonSuccessFinal(p, r); });
  onMessage<GlobalFailureResponse>(&InviteClientProceedingState::instance(),
    [handleProceedingNonSuccessFinal](Base& p, const GlobalFailureResponse& r) { handleProceedingNonSuccessFinal(p, r); });

  // -- Completed: absorbs retransmitted final responses by resending the ACK --

  auto handleCompletedRetransmit = [sendAck](Base&, const SipResponse& r) { sendAck(r); };
  onMessage<RedirectionResponse>(&InviteClientCompletedState::instance(),
    [handleCompletedRetransmit](Base& p, const RedirectionResponse& r) { handleCompletedRetransmit(p, r); });
  onMessage<ClientErrorResponse>(&InviteClientCompletedState::instance(),
    [handleCompletedRetransmit](Base& p, const ClientErrorResponse& r) { handleCompletedRetransmit(p, r); });
  onMessage<ServerErrorResponse>(&InviteClientCompletedState::instance(),
    [handleCompletedRetransmit](Base& p, const ServerErrorResponse& r) { handleCompletedRetransmit(p, r); });
  onMessage<GlobalFailureResponse>(&InviteClientCompletedState::instance(),
    [handleCompletedRetransmit](Base& p, const GlobalFailureResponse& r) { handleCompletedRetransmit(p, r); });

  onMessage<TransactionWaitTimeoutEvent>(&InviteClientCompletedState::instance(),
    [](Base& p, const TransactionWaitTimeoutEvent&) {
      p.setCurrentState(&InviteClientTerminatedState::instance());
    });

  // -- Terminated: no entries registered. See the file comment -- a
  // message arriving here is silently discarded, not a protocol
  // violation.

  sendRequest(invite);
  setCurrentState(&InviteClientCallingState::instance());
}

} // namespace ModernCommon
