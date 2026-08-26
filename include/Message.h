/* -*- C++ -*- */
//
// Message.h
//

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

  /// A reference-counted handle to a heap-allocated Message. Downcasting
  /// to a concrete message type is std::dynamic_pointer_cast<ConcreteMsg>
  /// -- no separate typesafe handle wrapper needed.
  using MessageHandle = std::shared_ptr<Message>;

} // namespace ModernCommon

#endif // MODERNTEMPLATE_MESSAGE_H
