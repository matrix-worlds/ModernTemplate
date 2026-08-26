/* -*- C++ -*- */
//
// StateMachine.h
//
// Modern-C++ replacement for isdx's ISDX::BasicStateMachine +
// ISDX::StateMachine<S> (src/lib/isdxcommon/include/BasicStateMachine.h,
// StateMachine.h): the generic driver that holds "the current state" and
// runs onExit()/onEntry() hooks around every transition. isdx
// instantiates this once per protocol (SipConnectionStateMachine,
// ISUPCircuitStateMachine, ATMCircuitStateMachine, MGCSEndpointStateMachine,
// CPUManagerStateMachine, ProcessManagerStateMachine, ... -- eight-plus
// distinct instantiations in isdx/src/lib), and this class is carried
// over essentially unchanged: it was already the fully generic part.
// What each isdx protocol builds *on top of* this -- hand-written
// per-message dispatch -- is what Protocol.h newly generalizes; see that
// file's comment.
//
// Modernizations from isdx's version:
//
//  - The current-state pointer is `const S*`, matching State.h's
//    singleton-state recommendation (a singleton is inherently shared
//    and should not be mutated through it). isdx's setCurrentState()
//    took `const S*` but stored into a non-const `S* _currentState`,
//    requiring an internal const_cast; that workaround is gone because
//    there's no longer a mismatch to work around.
//  - The hardcoded `ISDX_LOG(...)` printf-style transition log is
//    replaced with an injectable `TransitionObserver`
//    (`std::function<void(name, oldStateName, newStateName)>`) --  the
//    same "callback instead of a fixed side effect" modernization used
//    for Data.h's RowCallback<> in place of ResultsProcessor<>
//    subclassing. Set one if you want transition logging; the default is
//    a silent no-op, so this header has no logging-framework dependency
//    at all (isdx's version depended on LogManager.h).
//  - `S` is constrained via State.h's `DerivedFromState` concept.

#ifndef MODERNTEMPLATE_STATEMACHINE_H
#define MODERNTEMPLATE_STATEMACHINE_H

#include "State.h"
#include <functional>
#include <sstream>
#include <string>
#include <utility>

namespace ModernCommon {

  /// Non-template base every StateMachine<S> derives from -- see State.h's
  /// StateMachineBase forward declaration for why State's onEntry()/
  /// onExit() hooks are written against this instead of StateMachine<S>
  /// directly.
  class StateMachineBase
  {
  public:
    explicit StateMachineBase(std::string name) : _name(std::move(name)) {}
    virtual ~StateMachineBase() = default;

    const std::string& getName() const { return _name; }

    virtual std::string getString() const
    {
      std::ostringstream ost;
      ost << "name=" << _name;
      return ost.str();
    }

    StateMachineBase(const StateMachineBase&) = delete;
    StateMachineBase& operator=(const StateMachineBase&) = delete;

  private:
    std::string _name;
  };

  /**
   * StateMachine
   *
   * Generic state machine driver: holds a pointer to the current state
   * (nullptr = "undefined", the same convention isdx used) and runs the
   * onExit()/onEntry() hooks around every setCurrentState() call.
   */
  template<class S>
    requires DerivedFromState<S>
  class StateMachine : public StateMachineBase
  {
  public:
    /// Invoked after every transition, with (this machine's name, the old
    /// state's name or "UNDEFINED_STATE", the new state's name or
    /// "UNDEFINED_STATE"). Defaulted to a no-op.
    using TransitionObserver =
      std::function<void(const std::string&, const std::string&, const std::string&)>;

    explicit StateMachine(std::string name) : StateMachineBase(std::move(name)) {}

    const S* getCurrentState() const { return _currentState; }

    /// Transitions to `state` (nullptr is valid: it means "no current
    /// state"). Runs, in order: the old state's onExit(), the transition
    /// observer (if any), the actual pointer swap, then the new state's
    /// onEntry() -- the same sequence isdx's StateMachine<S>::
    /// setCurrentState() used, so onEntry()/onExit() overrides that
    /// change the current state again as part of their action behave the
    /// same way here as they did there.
    void setCurrentState(const S* state)
    {
      static const std::string kUndefined = "UNDEFINED_STATE";

      std::string oldName = (_currentState != nullptr) ? _currentState->getName() : kUndefined;
      const S* oldState = _currentState;

      if (_currentState != nullptr)
      {
        _currentState->onExit(*this, state);
      }

      std::string newName = (state != nullptr) ? state->getName() : kUndefined;

      if (_observer)
      {
        _observer(getName(), oldName, newName);
      }

      _currentState = state;

      if (_currentState != nullptr)
      {
        _currentState->onEntry(*this, oldState);
      }
    }

    void setTransitionObserver(TransitionObserver observer) { _observer = std::move(observer); }

    std::string getString() const override
    {
      std::ostringstream ost;
      ost << "[StateMachine: " << StateMachineBase::getString() << ", currentState="
          << ((_currentState != nullptr) ? _currentState->getName() : std::string("UNDEFINED_STATE"))
          << "]";
      return ost.str();
    }

  private:
    const S* _currentState = nullptr;
    TransitionObserver _observer;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_STATEMACHINE_H
