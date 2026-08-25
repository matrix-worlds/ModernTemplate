/* -*- C++ -*- */
//
// CacheData.h
//
// Modern-C++ replacement for MyTemplate's CacheData.h. A snapshot cache
// sitting in front of a Table<>: reload() pulls every row through once
// and holds it in memory; lookups afterwards never touch the database.
//
// Two changes from MyTemplate's version:
//
//  1. Cached entries are held as std::shared_ptr<EntryT> inside a
//     SynchronizedMap<Key, std::shared_ptr<EntryT>> (Synchronized.h)
//     instead of MyTemplate's CommonMap<Key, RCObject<Entry>*> +
//     intrusive-refcounted RCPtr<Entry>. A lookup returns a
//     std::shared_ptr the caller can hold onto for as long as it likes;
//     nobody has to hand-manage a reference count, and reload() swapping
//     in a fresh map can't invalidate a shared_ptr a caller is still
//     holding (compare: the raw RCObject* pattern relied on every
//     holder going through RCPtr correctly to avoid a dangling pointer
//     after eviction).
//
//  2. There is exactly one CacheData<> template, not a separate
//     Constrained/Unconstrained pair. MyTemplate split those because its
//     CommonKey1..6 base types made "no constraint" and "one constraint"
//     look like different type shapes. Here a constraint is just a
//     `std::function<bool(const Constraint&, const EntryT&)>` predicate
//     supplied at construction (defaulted to "always true" -- i.e.
//     unconstrained) -- one class covers both cases. Constraint defaults
//     to the empty tag type Unconstrained (below), not void: `const
//     void&` isn't a legal parameter type, so a real (if empty) type is
//     needed to keep the unconstrained case an ordinary instantiation of
//     the same template rather than a special case requiring its own
//     partial specialization.
//
// One deliberate omission, not an oversight: MyTemplate's
// *SingleVersionSnapshotCacheData<> bakes a getInstance()/setInstance()/
// resetInstanceForTesting() singleton directly into the cache class
// itself. That's a second copy of the same Meyers-singleton idea already
// shown once in ObjectPool.h and Dump.h; baking it into every
// instantiation of CacheData<> as well would just be the pattern
// repeated a third time for no added benefit. A caller who wants one
// shared, process-wide WidgetCacheData reaches for the same one-line
// idiom used there: a function-local `static CacheData<...> instance;`
// (or, if it must be swappable for tests the way MyTemplate's
// setInstance()/resetInstanceForTesting() was, a
// `static std::unique_ptr<CacheData<...>>` the test can reset directly)
// -- rather than have this header carry that machinery for every cache,
// used or not.



#ifndef MODERNTEMPLATE_CACHEDATA_H
#define MODERNTEMPLATE_CACHEDATA_H

#include "Data.h"
#include "Synchronized.h"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ModernCommon {

  /// Default Constraint type: an empty tag, used when a cache has no
  /// secondary filter at all. See the file comment for why this exists
  /// instead of defaulting Constraint to void.
  struct Unconstrained {};

  /**
   * CacheData
   *
   * Key/EntryT: same meaning as Table<>'s. Constraint: the type of an
   * optional secondary filter (e.g. "only widgets in this category");
   * leave it defaulted to Unconstrained if the table has none.
   */
  template<class Key, class EntryT, class Constraint = Unconstrained>
  class CacheData
  {
  public:
    using Predicate = std::function<bool(const Constraint&, const EntryT&)>;

    /// `predicate` decides whether a row belongs under a given
    /// constraint value; irrelevant (and defaulted) when Constraint is
    /// Unconstrained.
    explicit CacheData(Predicate predicate = [](const Constraint&, const EntryT&) { return true; })
      : _predicate(std::move(predicate))
    {}

    /// Refill the cache from `table` in one pass. `table` is expected to
    /// already be connected (see DataFactory::setDefaultConnectionParams
    /// if it was default-constructed).
    template<class ConnectionPolicy>
    void reload(Table<Key, EntryT, ConnectionPolicy>& table)
    {
      SynchronizedMap<Key, std::shared_ptr<EntryT>> fresh;
      table.query([&](const EntryT& e) {
        fresh.addDataEntry(e.getKey(), std::make_shared<EntryT>(e));
        return true;
      });
      _map.reload(fresh);
    }

    /// Cached lookup by key -- never touches the database.
    std::shared_ptr<EntryT> lookup(const Key& key) const
    {
      auto found = _map.lookupDataEntry(key);
      return found ? *found : nullptr;
    }

    /// Every cached row matching `constraint` (or every row, called with
    /// no argument, when Constraint is Unconstrained / the predicate is
    /// trivial).
    std::vector<std::shared_ptr<EntryT>> lookupAll(const Constraint& constraint) const
    {
      std::vector<std::shared_ptr<EntryT>> results;
      for (auto& [key, entry] : _map.snapshot())
      {
        if (_predicate(constraint, *entry)) results.push_back(entry);
      }
      return results;
    }

    std::vector<std::shared_ptr<EntryT>> lookupAll() const
    {
      std::vector<std::shared_ptr<EntryT>> results;
      for (auto& [key, entry] : _map.snapshot()) results.push_back(entry);
      return results;
    }

    std::size_t size() const { return _map.size(); }

  private:
    SynchronizedMap<Key, std::shared_ptr<EntryT>> _map;
    Predicate _predicate;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_CACHEDATA_H
