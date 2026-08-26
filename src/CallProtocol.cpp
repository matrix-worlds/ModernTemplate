/* -*- C++ -*- */
//
// CallProtocol.cpp
//
// See CallProtocol.h. This is the "one place that lists the whole
// transition table" Protocol.h's file comment contrasts with isdx's
// approach of spreading the same information across N state subclasses'
// overridden virtuals.

#include "CallProtocol.h"
#include <stdexcept>

namespace ModernCommon {

CallProtocol::CallProtocol(std::string name)
  : ProtocolStateMachine<State, Message>(std::move(name))
{
  onMessage<SetupMessage>(
    &CallIdleState::instance(),
    [](ProtocolStateMachine<State, Message>& p, const SetupMessage&) {
      p.setCurrentState(&CallRingingState::instance());
    });

  onMessage<AnswerMessage>(
    &CallRingingState::instance(),
    [](ProtocolStateMachine<State, Message>& p, const AnswerMessage&) {
      p.setCurrentState(&CallConnectedState::instance());
    });

  onMessage<ReleaseMessage>(
    &CallRingingState::instance(),
    [](ProtocolStateMachine<State, Message>& p, const ReleaseMessage&) {
      p.setCurrentState(&CallIdleState::instance());
    });

  onMessage<ReleaseMessage>(
    &CallConnectedState::instance(),
    [](ProtocolStateMachine<State, Message>& p, const ReleaseMessage&) {
      p.setCurrentState(&CallIdleState::instance());
    });

  onUnhandled([](ProtocolStateMachine<State, Message>& p, const Message& msg) {
    const State* current = p.getCurrentState();
    throw std::logic_error(
      "CallProtocol: unexpected \"" + msg.getName() + "\" message in state \"" +
      (current != nullptr ? current->getName() : std::string("UNDEFINED_STATE")) + "\"");
  });

  setCurrentState(&CallIdleState::instance());
}

} // namespace ModernCommon
