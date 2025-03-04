// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define PYBIND11_DETAILED_ERROR_MESSAGES 1

#include <string>
#include <unordered_map>

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <pybind11/pybind11.h>

namespace Acts::Python {

/// Python context to store modules and other objects
struct Context {
  /// The module map, accessible by name
  std::unordered_map<std::string, pybind11::module_> modules;

  /// Retrieve a pybind11 module by name from the map
  ///
  /// @param name The name of the module
  ///
  /// @return retun the module from the map, throws an exception if not found
  pybind11::module_& get(const std::string& name) { return modules.at(name); }

  /// Retrieve a tuple of pybind11 modules by name from the map
  ///
  /// @param args The names of the modules
  ///
  /// @return return the modules from the map, throws an exception if not found
  template <typename... Args>
  auto get(Args&&... args)
    requires(sizeof...(Args) >= 2)
  {
    return std::make_tuple((modules.at(args))...);
  }
};

template <typename T, typename Ur, typename Ut>
void pythonRangeProperty(T& obj, const std::string& name, Ur Ut::* begin,
                         Ur Ut::* end) {
  obj.def_property(
      name.c_str(), [=](Ut& self) { return std::pair{self.*begin, self.*end}; },
      [=](Ut& self, std::pair<Ur, Ur> p) {
        self.*begin = p.first;
        self.*end = p.second;
      });
}

inline void patchClassesWithConfig(pybind11::module_& m) {
  pybind11::module::import("acts._adapter").attr("_patch_config")(m);
}

template <typename T>
void patchKwargsConstructor(T& c) {
  pybind11::module::import("acts._adapter").attr("_patchKwargsConstructor")(c);
}
}  // namespace Acts::Python

#define ACTS_PYTHON_MEMBER(name) \
  _binding_instance.def_readwrite(#name, &_struct_type::name)

#define ACTS_PYTHON_STRUCT_BEGIN(obj, cls)          \
  {                                                 \
    [[maybe_unused]] auto& _binding_instance = obj; \
    using _struct_type = cls;                       \
    do {                                            \
    } while (0)

#define ACTS_PYTHON_STRUCT_END() \
  }                              \
  do {                           \
  } while (0)
