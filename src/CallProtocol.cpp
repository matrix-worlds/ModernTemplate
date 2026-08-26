/* -*- C++ -*- */
//
// CallProtocol.cpp
//
// See CallProtocol.h. This constructor is the whole transition table,
// readable top to bottom -- the point of Protocol.h's design (see that
// file's comment).

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
