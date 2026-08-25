/* -*- C++ -*- */
//
// Dump.cpp
//
// See Dump.h.

#include "Dump.h"
#include <iostream>

namespace ModernCommon {

void
DiagnosticDumpRegistry::dump() const
{
  std::shared_lock lock(_mutex);

  std::cout << "\n------------------------------------------------\n";
  for (const auto& [name, obj] : _map)
  {
    std::cout << "- - - - - - - - - - - - - - - -\n"
              << "Name: \"" << name << "\", object: " << obj << "\n";
    obj->dump();
  }
  std::cout << "------------------------------------------------\n";
}

void
DiagnosticDumpRegistry::dump(const std::string& name) const
{
  std::shared_lock lock(_mutex);

  std::cout << "\n------------------------------------------------\n";
  auto [first, last] = _map.equal_range(name);
  for (auto it = first; it != last; ++it)
  {
    std::cout << "- - - - - - - - - - - - - - - -\n"
              << "Name: \"" << it->first << "\", object: " << it->second << "\n";
    it->second->dump();
  }
  std::cout << "------------------------------------------------\n";
}

void
DiagnosticDumpRegistry::registerObject(const std::string& name, Dumpable* obj)
{
  std::unique_lock lock(_mutex);
  _map.emplace(name, obj);
}

void
DiagnosticDumpRegistry::deregisterObject(Dumpable* obj)
{
  std::unique_lock lock(_mutex);
  for (auto it = _map.begin(); it != _map.end(); ++it)
  {
    if (it->second == obj)
    {
      _map.erase(it);
      return;
    }
  }
}

} // namespace ModernCommon
