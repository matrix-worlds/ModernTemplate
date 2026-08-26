/* -*- C++ -*- */
//
// Message.h
//
// Modern-C++ replacement for isdx's ISDX::PFA::Msg /
// ISDX::PFA::MsgHandleTemplate<C> (src/lib/isdxplat/include/Msg.h) and,
// in spirit, ISDX::EvtHandle<C> (isdxcommon/include/Event.h) -- the same
// "typesafe handle template" shape appears twice in isdx for two
// different base types (Msg, Event), each hand-written.
//
// isdx's Msg is an intrusively reference-counted RCObject<TSRefCount>
// (see MyTemplate/ModernTemplate's own RefCount.h/ObjectPool.h notes on
// that pattern), handed out via MsgHandle = RCPtr<Msg>. Concrete message
// types get their own typesafe handle by instantiating
// MsgHandleTemplate<ConcreteMsg>, whose getPtr() wraps a dynamic_cast --
// e.g. isdx's real SipConnectionStateMachine::handlePFASetupMsg() takes a
// `SetupMsgHandle&`, one such instantiation per PFA message type.
//
// Here, MessageHandle is a plain `std::shared_ptr<Message>`, and there is
// no handle-template class at all: downcasting to a concrete message
// type is `std::dynamic_pointer_cast<ConcreteMsg>(handle)`, which the
// standard library has provided since C++11 and does exactly what
// MsgHandleTemplate<C>::getPtr()'s hand-written dynamic_cast did. Every
// message type that used to need its own `FooMsgHandle` class
// (SetupMsgHandle, SetupAckMsgHandle, StatusMsgHandle, ... -- isdx has
// dozens) needs nothing extra now; std::dynamic_pointer_cast<Foo> is
// already typed by its template argument. Same story as CommonKey1..6
// collapsing into std::tuple<> (see ModernTemplate/doc/comparison.md) --
// a hand-written template whose whole job was reproducing something the
// standard library already provides for free.
//
// See Protocol.h for how a ProtocolStateMachine<> routes incoming
// messages to per-(state, message-type) handlers using the *static*
// (compile-time) type of a message, which sidesteps needing any handle
// downcast at all in the common case -- dynamic_pointer_cast remains
// available for call sites that receive a MessageHandle and don't
// already know its concrete type by construction.

#ifndef MODERNTEMPLATE_MESSAGE_H
#define MODERNTEMPLATE_MESSAGE_H

#include <memory>
#include <string>

namespace ModernCommon {

  /// Base class for protocol messages. Concrete message types (see
  /// Protocol.h, CallProtocol.h) derive from this and add whatever
  /// fields their wire format carries.
  class Message
  {
  public:
    virtual ~Message() = default;

    /// Human-readable message name (e.g. "Setup", "IAM") -- used for
    /// diagnostics and in ProtocolStateMachine's default "unhandled
    /// message" error text (Protocol.h).
    virtual std::string getName() const = 0;

    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;

  protected:
    Message() = default;
  };

  /// A reference-counted handle to a heap-allocated Message. Plain
  /// std::shared_ptr -- see the file comment for why no
  /// MsgHandleTemplate<C>-shaped wrapper is needed alongside it.
  using MessageHandle = std::shared_ptr<Message>;

} // namespace ModernCommon

#endif // MODERNTEMPLATE_MESSAGE_H
