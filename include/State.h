/* -*- C++ -*- */
//
// State.h
//
// Modern-C++ replacement for isdx's ISDX::State (src/lib/isdxcommon/
// include/State.h): the abstract base every concrete state of a
// StateMachine<> (StateMachine.h) derives from.
//
// Two modernizations from isdx's version:
//
//  1. isdx forces "you must subclass this" via a *pure virtual
//     destructor* (`virtual ~State() = 0;`, defined out-of-line as an
//     empty body) despite State having no other pure virtual method.
//     That's a C++98-era idiom for an otherwise-concrete-looking class.
//     Here the constructor is simply `protected` instead -- the same
//     "can't instantiate State directly, subclasses can" guarantee,
//     without the awkward "pure virtual destructor with a body" pattern.
//
//  2. isdx's State has virtual `operator==`/`operator!=` comparing by
//     name, overridable per subclass. This header drops them entirely
//     and recommends instead that concrete states be process-wide
//     Meyers singletons (`static const ConcreteState& instance()`),
//     the same idiom already used for ObjectPool<T>::getInstance() and
//     DiagnosticDumpRegistry::instance() elsewhere in this project. Once
//     a state is a singleton, plain pointer/reference identity (`&a ==
//     &b`) *is* state equality -- correct, free, and not overridable
//     into inconsistency the way isdx's virtual operator==/!= could be.
//     Nothing here *requires* singleton states (a state machine may
//     still own private per-instance state objects, as isdx's real
//     protocol state machines mostly do -- see the survey this project's
//     doc/state-protocol.md is built from), but the type doesn't carry
//     comparison machinery that only pays off for the singleton case.
//
// See StateMachine.h for the driver this state is plugged into, and
// Protocol.h for how concrete protocols route incoming messages to
// per-(state, message-type) handlers.

#ifndef MODERNTEMPLATE_STATE_H
#define MODERNTEMPLATE_STATE_H

#include <string>
#include <type_traits>
#include <utility>

namespace ModernCommon {

  /// Non-template handle a State's onEntry()/onExit() hooks receive --
  /// see StateMachine.h. Lets those hooks reference "the state machine
  /// that's transitioning" without State itself needing to be templated
  /// on the concrete state machine's state type.
  class StateMachineBase;

  /// Base class for states driven by a StateMachine<S> (StateMachine.h).
  class State
  {
  public:
    virtual ~State() = default;

    /// Invoked by the owning StateMachine<S> immediately after this state
    /// becomes current. `oldState` is the state transitioned from, or
    /// nullptr if this is the machine's first state. May itself change
    /// the machine's current state as part of its action.
    virtual void onEntry(StateMachineBase& sm, const State* oldState) const {}

    /// Invoked immediately before this state stops being current.
    /// `newState` is the state about to become current. Unlike onEntry(),
    /// must not change the machine's current state.
    virtual void onExit(StateMachineBase& sm, const State* newState) const {}

    const std::string& getName() const { return _name; }

    State(const State&) = delete;
    State& operator=(const State&) = delete;

  protected:
    explicit State(std::string name) : _name(std::move(name)) {}

  private:
    const std::string _name;
  };

  /// Constraint used by StateMachine<S> (StateMachine.h) and
  /// ProtocolStateMachine<S,MessageBase> (Protocol.h): S must derive from
  /// State. A real (if minor) improvement on isdx, where nothing stopped
  /// `StateMachine<SomeUnrelatedType>` from being written and failing,
  /// confusingly, only once setCurrentState() tried to call getName() on
  /// it.
  template<class S>
  concept DerivedFromState = std::is_base_of_v<State, S>;

} // namespace ModernCommon

#endif // MODERNTEMPLATE_STATE_H
