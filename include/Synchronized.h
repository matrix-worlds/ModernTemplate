/* -*- C++ -*- */
//
// Synchronized.h
//
// Modern-C++ replacement for MyTemplate's CommonMap<Key,Data,Mutex>
// (a std::map hand-wrapped with an ACE-then-std::recursive_mutex,
// exposing addDataEntry()/lookupDataEntry()/removeDataEntry()/reload()).
//
// Two pieces here, mirroring the comparison in doc/comparison.md:
//
// Synchronized<T, Mutex>: a *generic* "lock this thing before you touch
// it" wrapper for any T, not just a map -- the same idea as Facebook
// Folly's folly::Synchronized<T>. All access goes through withLock(),
// so there's no way to reach _value without holding the mutex; compare
// to CommonMap<>, which had every method separately taking the same
// lock, all easy to get right individually but with no single
// mechanism stopping a future method from forgetting to.
//
// SynchronizedMap<Key,Data,Mutex>: the CommonMap<>-shaped convenience
// built on top of Synchronized<std::map<Key,Data>>, for direct
// comparison with the original API. Two API-level modernizations here:
//  - lookupDataEntry() returns std::optional<Data> instead of MyTemplate's
//    bool-return-plus-Data&-out-param.
//  - reload() takes another SynchronizedMap by const reference and
//    copies its contents out while holding *its* lock, rather than
//    (as the original did) reaching into a sibling instance's private
//    std::map directly -- both give the same atomic-swap behavior
//    CacheData.h's snapshot cache relies on, but this one doesn't need
//    the two instances to be the same class to be friends with each
//    other's internals.

#ifndef MODERNTEMPLATE_SYNCHRONIZED_H
#define MODERNTEMPLATE_SYNCHRONIZED_H

#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace ModernCommon {

  template<class T, class Mutex = std::mutex>
  class Synchronized
  {
  public:
    Synchronized() = default;
    explicit Synchronized(T value) : _value(std::move(value)) {}

    /// Runs fn(T&) while holding the lock; fn's return value (if any)
    /// is returned. The lock is never visible/reachable outside fn.
    template<class Fn>
    decltype(auto) withLock(Fn&& fn)
    {
      std::lock_guard<Mutex> guard(_mutex);
      return fn(_value);
    }

    template<class Fn>
    decltype(auto) withLock(Fn&& fn) const
    {
      std::lock_guard<Mutex> guard(_mutex);
      return fn(_value);
    }

  private:
    mutable Mutex _mutex;
    T _value{};
  };

  template<class Key, class Data, class Mutex = std::mutex>
  class SynchronizedMap
  {
  public:
    using MapType = std::map<Key, Data>;

    /// Inserts (key, data) if key is absent. Returns false (and leaves
    /// the map unchanged) if key was already present.
    bool addDataEntry(const Key& key, Data data)
    {
      return _map.withLock([&](MapType& m) {
        return m.emplace(key, std::move(data)).second;
      });
    }

    /// Returns a copy of the entry for key, or std::nullopt if absent.
    std::optional<Data> lookupDataEntry(const Key& key) const
    {
      return _map.withLock([&](const MapType& m) -> std::optional<Data> {
        auto it = m.find(key);
        if (it == m.end()) return std::nullopt;
        return it->second;
      });
    }

    void removeDataEntry(const Key& key)
    {
      _map.withLock([&](MapType& m) { m.erase(key); });
    }

    std::size_t size() const
    {
      return _map.withLock([](const MapType& m) { return m.size(); });
    }

    /// Replaces this map's contents with a snapshot of `other`'s,
    /// atomically from this map's point of view (readers see either the
    /// old contents in full or the new contents in full, never a mix).
    void reload(const SynchronizedMap& other)
    {
      MapType snapshot = other._map.withLock([](const MapType& m) { return m; });
      _map.withLock([&](MapType& m) { m = std::move(snapshot); });
    }

    /// Copies out every (key, data) pair. Used by dump()-style callers
    /// that want to print the whole map without holding the lock while
    /// doing I/O.
    std::vector<std::pair<Key, Data>> snapshot() const
    {
      return _map.withLock([](const MapType& m) {
        return std::vector<std::pair<Key, Data>>(m.begin(), m.end());
      });
    }

  private:
    Synchronized<MapType, Mutex> _map;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_SYNCHRONIZED_H
