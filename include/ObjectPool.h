/* -*- C++ -*- */
//
// ObjectPool.h
//
// Modern-C++ replacement for MyTemplate's ObjectPool<T> (and, one level
// further back, RefCount.h's TSRefCount/RCObject<>/RCPtr<>, which that
// pool's Handle was built on).
//
// MyTemplate's design required T to publicly inherit from
// RCObject<TSRefCount> so it could be wrapped in the hand-written
// intrusive smart pointer RCPtr<T>. Here, Handle is just a
// std::shared_ptr<T> with a custom deleter that, instead of actually
// deleting the object, resets() it and pushes it back onto the pool's
// free list -- a well-known modern C++ idiom (see e.g. the "object pool
// with shared_ptr" pattern discussed widely for exactly this reason).
// Consequences:
//  - T no longer needs to inherit from anything. Any default-
//    constructible type with a reset() method works -- the intrusive
//    inheritance requirement, and the whole hand-rolled reference-
//    counting machinery in RefCount.h, are simply gone.
//  - The pool itself must be reached via a std::shared_ptr<ObjectPool<T>>
//    (create() below), because the deleter captures a std::weak_ptr
//    back to the pool: if every ObjectPool<T> reference is gone before
//    a checked-out Handle is released, the deleter notices the weak_ptr
//    can't be locked and just deletes the object normally instead of
//    dereferencing a dead pool -- so, unlike a bare `ObjectPool<T>*`
//    captured by raw pointer, this can't crash on pool-outlives-handle
//    ordering mistakes.
//
// getInstance() is still a Meyers singleton per T -- see Dump.h's file
// comment for why that's the preferred modern form (no manual double-
// checked locking needed).

#ifndef MODERNTEMPLATE_OBJECTPOOL_H
#define MODERNTEMPLATE_OBJECTPOOL_H

#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace ModernCommon {

  template<class T>
  class ObjectPool : public std::enable_shared_from_this<ObjectPool<T>>
  {
    struct PrivateTag {};

  public:
    using Handle = std::shared_ptr<T>;

    /// The only way to obtain a pool: creates a pool pre-populated with
    /// `initialSize` objects; acquire() grows it by `incrementStep` at
    /// a time once exhausted.
    static std::shared_ptr<ObjectPool<T>> create(std::size_t initialSize = 10,
                                                  std::size_t incrementStep = 3)
    {
      auto pool = std::make_shared<ObjectPool<T>>(PrivateTag{}, incrementStep);
      std::lock_guard<std::mutex> guard(pool->_mutex);
      for (std::size_t i = 0; i < initialSize; ++i)
      {
        pool->_free.push_back(std::make_unique<T>());
      }
      return pool;
    }

    /// Public only so std::make_shared<ObjectPool<T>>() can call it;
    /// use create() instead.
    ObjectPool(PrivateTag, std::size_t incrementStep) : _incrementStep(incrementStep) {}

    /// Take an object out of the pool, growing the pool first if it is
    /// currently empty. The returned Handle resets() and returns the
    /// object to this pool automatically when its last copy is dropped.
    Handle acquire()
    {
      std::unique_ptr<T> owned;
      {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_free.empty()) growLocked();
        owned = std::move(_free.back());
        _free.pop_back();
      }

      std::weak_ptr<ObjectPool<T>> weakSelf = this->weak_from_this();
      T* raw = owned.release();
      return Handle(raw, [weakSelf](T* p) {
        if (auto pool = weakSelf.lock())
        {
          p->reset();
          std::lock_guard<std::mutex> guard(pool->_mutex);
          pool->_free.push_back(std::unique_ptr<T>(p));
        }
        else
        {
          delete p; // pool is gone; nothing left to return it to
        }
      });
    }

    /// Number of objects currently available (not checked out).
    std::size_t getPoolSize() const
    {
      std::lock_guard<std::mutex> guard(_mutex);
      return _free.size();
    }

    std::string getString() const
    {
      std::ostringstream ost;
      ost << " [ ObjectPool: size = " << getPoolSize() << " ] ";
      return ost.str();
    }

    /// One shared pool per pooled type T, created on first use.
    static ObjectPool<T>& getInstance()
    {
      static std::shared_ptr<ObjectPool<T>> instance = create();
      return *instance;
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

  private:
    void growLocked()
    {
      for (std::size_t i = 0; i < _incrementStep; ++i)
      {
        _free.push_back(std::make_unique<T>());
      }
    }

    mutable std::mutex _mutex;
    std::vector<std::unique_ptr<T>> _free;
    std::size_t _incrementStep;
  };

} // namespace ModernCommon

#endif // MODERNTEMPLATE_OBJECTPOOL_H
