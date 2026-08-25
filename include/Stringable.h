/* -*- C++ -*- */
//
// Stringable.h
//
// Kept as a small virtual interface (getString() + free operator<<),
// same idea as MyTemplate's Stringable.h. This is one of the few pieces
// in this project that a "just use modern std facilities" pass doesn't
// replace: Dump.h's DiagnosticDumpRegistry needs a heterogeneous
// *runtime* collection of "things that can print themselves"
// (std::vector<Dumpable*> holding all sorts of unrelated concrete
// types), which needs actual runtime polymorphism (a common base with
// a virtual function) -- a compile-time-only mechanism like a C++20
// concept can constrain a single call site's argument type, but can't
// by itself give you a container that holds many different concrete
// types behind one interface. See doc/comparison.md for the fuller
// writeup of what did and didn't get modernized here.

#ifndef MODERNTEMPLATE_STRINGABLE_H
#define MODERNTEMPLATE_STRINGABLE_H

#include <iostream>
#include <string>

namespace ModernCommon {

  class Stringable {
  public:
    virtual ~Stringable() = default;
    virtual std::string getString() const = 0;
  };

  inline std::ostream& operator<<(std::ostream& ost, const Stringable& obj)
  {
    return ost << obj.getString();
  }

} // namespace ModernCommon

#endif // MODERNTEMPLATE_STRINGABLE_H
