/* -*- C++ -*- */
//
// Dump.h
//
// Modern-C++ version of MyTemplate's Dump.h (Dumpable/DiagnosticDumpRegistry).
// No standard-library type replaces this outright -- see doc/comparison.md
// -- but two pieces of it modernize cleanly:
//
//  - getInstance() used to be a hand-written double-checked-locking
//    singleton (check for null, lock, check again, allocate). Since
//    C++11, a function-local static variable's initialization is
//    guaranteed thread-safe by the language itself -- this is the
//    "Meyers singleton" -- so the whole manual locking dance collapses
//    into one line.
//  - The registry's map is now guarded by std::shared_mutex: dump()ing
//    the whole registry (read-only traffic) can run concurrently with
//    other dump() calls, only register/deregister need the exclusive
//    lock. MyTemplate's version (and the original this project is all
//    ultimately descended from) used one exclusive lock for everything.

#ifndef MODERNTEMPLATE_DUMP_H
#define MODERNTEMPLATE_DUMP_H

#include <map>
#include <shared_mutex>
#include <string>

namespace ModernCommon {

  /// Base class for objects capable of producing a diagnostic dump.
  class Dumpable
  {
  public:
    virtual ~Dumpable() = default;
    virtual void dump() const = 0;
  };

  /// Thread-safe, process-wide singleton registry of Dumpable objects.
  class DiagnosticDumpRegistry
  {
  public:
    static DiagnosticDumpRegistry& instance()
    {
      static DiagnosticDumpRegistry registry; // Meyers singleton
      return registry;
    }

    void dump() const;
    void dump(const std::string& name) const;

    void registerObject(const std::string& name, Dumpable* obj);

    /// Removes the first registration found for `obj`. A no-op if `obj`
    /// was never registered.
    void deregisterObject(Dumpable* obj);

    std::size_t getNumberOfRegisteredObjects() const
    {
      std::shared_lock lock(_mutex);
      return _map.size();
    }

    DiagnosticDumpRegistry(const DiagnosticDumpRegistry&) = delete;
    DiagnosticDumpRegistry& operator=(const DiagnosticDumpRegistry&) = delete;

  private:
    DiagnosticDumpRegistry() = default;

    using DumpableMap = std::multimap<std::string, Dumpable*>;

    mutable std::shared_mutex _mutex;
    DumpableMap _map;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_DUMP_H
