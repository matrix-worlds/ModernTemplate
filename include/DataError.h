/* -*- C++ -*- */
//
// DataError.h
//
// Same four exceptions as MyTemplate's DataError.h. Nothing here gets
// "modernized" -- std::runtime_error subclasses are already the modern,
// idiomatic C++ answer for this.

#ifndef MODERNTEMPLATE_DATAERROR_H
#define MODERNTEMPLATE_DATAERROR_H

#include <stdexcept>
#include <string>

namespace ModernCommon {

  class ServiceUnavailable : public std::runtime_error
  {
  public:
    ServiceUnavailable() : runtime_error("service unavailable") {}
    explicit ServiceUnavailable(const std::string& msg) : runtime_error(msg) {}
  };

  class DuplicateEntry : public std::runtime_error
  {
  public:
    DuplicateEntry() : runtime_error("duplicate entry") {}
    explicit DuplicateEntry(const std::string& msg) : runtime_error(msg) {}
  };

  class EntryNotFound : public std::runtime_error
  {
  public:
    EntryNotFound() : runtime_error("entry not found") {}
    explicit EntryNotFound(const std::string& msg) : runtime_error(msg) {}
  };

  class DependencyViolation : public std::runtime_error
  {
  public:
    DependencyViolation() : runtime_error("dependency violation") {}
    explicit DependencyViolation(const std::string& msg) : runtime_error(msg) {}
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_DATAERROR_H
