/* -*- C++ -*- */
//
// ModernType.h
//
// Modern-C++ replacement for MyTemplate's BasicType<Type> (which hand-
// wrote operator==/!=/</> against an internal Type value): C++20's
// defaulted operator<=> generates all six comparison operators from a
// single declaration, as long as Type itself supports <=> (every
// built-in arithmetic type and std::string do, since C++20).
//
// Unlike BasicType<> (and this project's own Stringable.h -- see its
// file comment), ModernType<> does *not* inherit an abstract getString()
// interface: nothing needs to hold a heterogeneous collection of "any
// ModernType<>" through a common base pointer, so there's no reason to
// pay for a vtable here. getString() is a plain, non-virtual member.
// (Also, mechanically: a class with a polymorphic base whose own
// operator<=> isn't defaulted/available makes the *derived* class's
// defaulted operator<=> implicitly deleted -- comparing the base
// subobject has no valid operator to call. Dropping the base sidesteps
// that entirely, on top of being the right call anyway.)

#ifndef MODERNTEMPLATE_MODERNTYPE_H
#define MODERNTEMPLATE_MODERNTYPE_H

#include <compare>
#include <sstream>
#include <string>
#include <utility>

namespace ModernCommon {

  template<class Type>
  class ModernType
  {
  public:
    ModernType() = default;
    explicit ModernType(Type t) : _t(std::move(t)) {}

    /// One declaration, and the compiler synthesizes ==, !=, <, >, <=,
    /// >= by comparing _t -- the six operators MyTemplate's BasicType<>
    /// wrote out by hand.
    friend auto operator<=>(const ModernType&, const ModernType&) = default;
    friend bool operator==(const ModernType&, const ModernType&) = default;

    void setValue(Type t) { _t = std::move(t); }
    const Type& getValue() const { return _t; }

    std::string getString() const
    {
      std::ostringstream ost;
      ost << _t;
      return ost.str();
    }

  protected:
    Type _t{};

    /// Helper for a subclass's getString() override, formatted like
    /// " [ <typeName>: <value> ] ".
    std::string print(const std::string& typeName) const
    {
      std::ostringstream ost;
      ost << " [ " << typeName << ": " << _t << " ] ";
      return ost.str();
    }
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_MODERNTYPE_H
