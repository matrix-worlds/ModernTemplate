/* -*- C++ -*- */
//
// SipTransaction.h
//
// InviteClientTransaction: RFC 3261 SS17.1.1 (Figure 5), the INVITE
// client transaction state machine, built entirely from State.h/
// StateMachine.h/Message.h/Protocol.h/SipMessage.h -- the "can SIP's
// protocol be implemented via these templates" demonstration.
//
// States: Calling, Proceeding, Completed, Terminated. Deliberately out
// of scope: Timer A (INVITE retransmission over unreliable transport
// only) -- this transaction assumes something else owns retransmission
// scheduling and reports it here as a TransportErrorEvent/
// TransactionTimeoutEvent if it gives up, the same way SipResponse
// messages arrive from whatever owns the transport. Timer B (transaction
// timeout) and Timer D (wait for retransmissions in Completed) are
// modeled as ordinary Message subtypes fed to receive(), same as a real
// response -- from this state machine's point of view a timer firing and
// a message arriving are the same kind of event.
//
// Unlike CallProtocol.h's onUnhandled() (which throws on a protocol
// violation), this transaction never calls onUnhandled() at all: the
// inherited default is a silent no-op, which is the RFC-correct
// behavior for a message arriving after Terminated (e.g. a retransmitted
// final response after Timer D already fired) -- that's expected, not an
// error.

#ifndef MODERNTEMPLATE_SIPTRANSACTION_H
#define MODERNTEMPLATE_SIPTRANSACTION_H

#include "Message.h"
#include "Protocol.h"
#include "SipMessage.h"
#include "State.h"
#include <functional>
#include <string>

namespace ModernCommon {

  // ---- Non-response events -------------------------------------------

  class TransportErrorEvent : public Message
  {
  public:
    explicit TransportErrorEvent(std::string reason) : _reason(std::move(reason)) {}
    std::string getName() const override { return "TransportError"; }
    const std::string& getReason() const { return _reason; }

  private:
    std::string _reason;
  };

  /// Stands in for Timer B (transaction timeout) firing.
  class TransactionTimeoutEvent : public Message
  {
  public:
    std::string getName() const override { return "TransactionTimeout"; }
  };

  /// Stands in for Timer D (wait for response retransmissions) firing.
  class TransactionWaitTimeoutEvent : public Message
  {
  public:
    std::string getName() const override { return "TransactionWaitTimeout"; }
  };

  // ---- States -----------------------------------------------------------

  class InviteClientCallingState : public State
  {
  public:
    static const InviteClientCallingState& instance() { static const InviteClientCallingState s; return s; }
  private:
    InviteClientCallingState() : State("Calling") {}
  };

  class InviteClientProceedingState : public State
  {
  public:
    static const InviteClientProceedingState& instance() { static const InviteClientProceedingState s; return s; }
  private:
    InviteClientProceedingState() : State("Proceeding") {}
  };

  class InviteClientCompletedState : public State
  {
  public:
    static const InviteClientCompletedState& instance() { static const InviteClientCompletedState s; return s; }
  private:
    InviteClientCompletedState() : State("Completed") {}
  };

  class InviteClientTerminatedState : public State
  {
  public:
    static const InviteClientTerminatedState& instance() { static const InviteClientTerminatedState s; return s; }
  private:
    InviteClientTerminatedState() : State("Terminated") {}
  };

  // ---- Transaction ------------------------------------------------------

  class InviteClientTransaction : public ProtocolStateMachine<State, Message>
  {
  public:
    using SendRequest   = std::function<void(const SipRequest&)>;
    using SendAck        = std::function<void(const SipResponse&)>;
    using NotifyTU        = std::function<void(const SipResponse&)>;
    using NotifyTUError    = std::function<void(const std::string&)>;

    /// Sends `invite` via `sendRequest` once and enters Calling.
    /// `sendAck` is invoked to (re)send the ACK this transaction owns for
    /// non-2xx final responses (2xx ACKs are the TU/dialog layer's job,
    /// not this transaction's -- see RFC 3261 SS17.1.1.3). `notifyTU`
    /// passes every response up to the transaction user; `notifyTUError`
    /// reports Timer B or a transport error.
    InviteClientTransaction(std::string name,
                             const InviteRequest& invite,
                             SendRequest sendRequest,
                             SendAck sendAck,
                             NotifyTU notifyTU,
                             NotifyTUError notifyTUError);
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_SIPTRANSACTION_H
