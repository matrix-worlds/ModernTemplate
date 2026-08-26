/* -*- C++ -*- */
//
// StateMachine.h
//

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
   * (nullptr means "undefined") and runs the onExit()/onEntry() hooks
   * around every setCurrentState() call.
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
    /// onEntry() -- so onEntry()/onExit() overrides that change the
    /// current state again as part of their action still behave
    /// predictably.
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
