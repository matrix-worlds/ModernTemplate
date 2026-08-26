/* -*- C++ -*- */
//
// Protocol.h
//
// ProtocolStateMachine<S, MessageBase>: a state machine whose
// transitions are driven by incoming protocol messages. A naive
// implementation of "dispatch this message type in this state to this
// behavior" tends to end up as one virtual method per message type,
// overridden per state subclass -- each state overrides whichever
// methods apply to it and silently inherits a no-op for the rest. The
// (state x message-type) dispatch table that describes only exists
// implicitly then, spread across every state subclass's vtable: there's
// no single place to read it, and nothing catches a missing or typo'd
// combination at compile time -- only a missing override silently doing
// nothing at runtime.
//
// ProtocolStateMachine<S, MessageBase> makes that table an actual,
// explicit, inspectable value: a std::map<(state, message-type),
// handler>, built once via onMessage<ConcreteMsg>(state, handler) calls
// and driven by receive(msg). "Which messages does this protocol handle
// in which states" becomes a question you can answer by reading the
// registration calls in one place (see CallProtocol.cpp for a worked
// example) or, at runtime, by calling getTransitionTableSize() -- rather
// than by reading every state subclass's worth of overridden virtuals.
//
// The message-type key is std::type_index(typeid(msg)) -- the concrete
// C++ type of the message identifies it, so no protocol needs to invent
// and maintain its own message-type enum just to make messages
// identifiable; RTTI already provides a unique, collision-free
// identifier for every concrete Message subclass.

#ifndef MODERNTEMPLATE_PROTOCOL_H
#define MODERNTEMPLATE_PROTOCOL_H

#include "Message.h"
#include "State.h"
#include "StateMachine.h"
#include <functional>
#include <map>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

namespace ModernCommon {

  template<class S, class MessageBase = Message>
    requires DerivedFromState<S>
  class ProtocolStateMachine : public StateMachine<S>
  {
  public:
    using Handler = std::function<void(ProtocolStateMachine&, const MessageBase&)>;

    explicit ProtocolStateMachine(std::string name) : StateMachine<S>(std::move(name)) {}

    /// Registers `handler` to run when receive() is called with a message
    /// whose *runtime* type is exactly `ConcreteMsg` while this machine's
    /// current state is `state`. `state` is compared by pointer identity
    /// against getCurrentState() at dispatch time -- see State.h's
    /// singleton-state recommendation; `state` must outlive this
    /// registration (a process-wide singleton always does).
    template<class ConcreteMsg>
    void onMessage(const S* state, std::function<void(ProtocolStateMachine&, const ConcreteMsg&)> handler)
    {
      static_assert(std::is_base_of_v<MessageBase, ConcreteMsg>,
                    "ConcreteMsg must derive from MessageBase");

      _table[Key{state, std::type_index(typeid(ConcreteMsg))}] =
        [handler = std::move(handler)](ProtocolStateMachine& psm, const MessageBase& msg) {
          handler(psm, static_cast<const ConcreteMsg&>(msg));
        };
    }

    /// Registers a fallback invoked by receive() when there's no entry
    /// for (getCurrentState(), typeid(msg)). Defaulted to a silent no-op;
    /// CallProtocol.cpp's worked example sets one that throws, treating
    /// an unexpected message as a protocol violation.
    void onUnhandled(Handler handler) { _unhandled = std::move(handler); }

    /// Looks up the handler registered for (getCurrentState(), the
    /// runtime type of `msg`) and invokes it, or the unhandled hook (if
    /// any) when there isn't one. This is the message-driven-transition
    /// dispatch this template exists to generalize -- see the file
    /// comment.
    void receive(const MessageBase& msg)
    {
      auto it = _table.find(Key{this->getCurrentState(), std::type_index(typeid(msg))});
      if (it != _table.end())
      {
        it->second(*this, msg);
      }
      else if (_unhandled)
      {
        _unhandled(*this, msg);
      }
    }

    /// Number of registered (state, message-type) entries -- lets a test
    /// assert on the shape of a protocol's transition table without
    /// exercising every entry by dispatch.
    std::size_t getTransitionTableSize() const { return _table.size(); }

  private:
    using Key = std::pair<const S*, std::type_index>;

    std::map<Key, Handler> _table;
    Handler _unhandled;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_PROTOCOL_H
