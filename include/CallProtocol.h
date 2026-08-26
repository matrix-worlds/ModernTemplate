/* -*- C++ -*- */
//
// CallProtocol.h
//
// A worked example of State.h/StateMachine.h/Message.h/Protocol.h
// working together: a minimal call-setup protocol, in the same spirit
// as isdx's real SipConnectionStateMachine/ISUPCircuitStateMachine (see
// Protocol.h's file comment) but reduced to the essentials --
// three states, three message types, four transitions:
//
//   Idle --Setup--> Ringing --Answer--> Connected
//   Ringing --Release--> Idle
//   Connected --Release--> Idle
//
// Any other message received in a given state (e.g. a second Setup
// while already Ringing) is a protocol violation: CallProtocol's
// constructor registers an onUnhandled() hook (Protocol.h) that throws
// std::logic_error naming the offending state and message, rather than
// silently doing nothing the way an isdx state subclass that didn't
// override the corresponding handleXXXMsg() would.
//
// States are Meyers singletons (State.h's recommended pattern): there is
// exactly one CallIdleState, one CallRingingState, one
// CallConnectedState for the whole process, shared by every CallProtocol
// instance -- correct because none of them carry any per-call data, the
// same as isdx's real SipConnectionIdleState/SipConnectionSetupState/...
// (see doc/state-protocol.md for the isdx source read that established
// this).

#ifndef MODERNTEMPLATE_CALLPROTOCOL_H
#define MODERNTEMPLATE_CALLPROTOCOL_H

#include "Message.h"
#include "Protocol.h"
#include "State.h"
#include <string>

namespace ModernCommon {

  // ---- Messages -------------------------------------------------------

  /// Requests a new call from `caller` to `callee`. Valid only in Idle.
  class SetupMessage : public Message
  {
  public:
    SetupMessage(std::string caller, std::string callee)
      : _caller(std::move(caller)), _callee(std::move(callee))
    {}

    std::string getName() const override { return "Setup"; }
    const std::string& getCaller() const { return _caller; }
    const std::string& getCallee() const { return _callee; }

  private:
    std::string _caller;
    std::string _callee;
  };

  /// Callee has answered. Valid only in Ringing.
  class AnswerMessage : public Message
  {
  public:
    std::string getName() const override { return "Answer"; }
  };

  /// Either party has hung up (or the call was rejected/abandoned before
  /// being answered). Valid in Ringing or Connected.
  class ReleaseMessage : public Message
  {
  public:
    explicit ReleaseMessage(std::string reason = "normal") : _reason(std::move(reason)) {}

    std::string getName() const override { return "Release"; }
    const std::string& getReason() const { return _reason; }

  private:
    std::string _reason;
  };

  // ---- States -----------------------------------------------------------

  class CallIdleState : public State
  {
  public:
    static const CallIdleState& instance()
    {
      static const CallIdleState s;
      return s;
    }

  private:
    CallIdleState() : State("Idle") {}
  };

  class CallRingingState : public State
  {
  public:
    static const CallRingingState& instance()
    {
      static const CallRingingState s;
      return s;
    }

  private:
    CallRingingState() : State("Ringing") {}
  };

  class CallConnectedState : public State
  {
  public:
    static const CallConnectedState& instance()
    {
      static const CallConnectedState s;
      return s;
    }

  private:
    CallConnectedState() : State("Connected") {}
  };

  // ---- Protocol -----------------------------------------------------------

  /// ProtocolStateMachine<State, Message> wired up for the call-setup
  /// protocol described in the file comment. Starts in CallIdleState.
  class CallProtocol : public ProtocolStateMachine<State, Message>
  {
  public:
    explicit CallProtocol(std::string name);
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_CALLPROTOCOL_H
