// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <nlohmann/json.hpp>

namespace Acts {

/// Convenience functions inside Acts.
///
/// The 'to_json', 'from_json' naming convention is
/// given by the nlohman library, this is a simple wrapper
/// into Acts-speak.
///
/// @tparam object_t the type of the object to be converter
///
/// @param o The object to be converted into json
///
/// @return a valid json object
template <typename object_t>
nlohmann::json toJson(const object_t& o) {
  nlohmann::json j;
  to_json(j, o);
  return j;
}

}  // namespace Acts
