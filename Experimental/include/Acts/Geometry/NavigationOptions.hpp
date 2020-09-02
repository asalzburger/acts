// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <climits>
#include "Acts/Surfaces/BoundaryCheck.hpp"
#include "Acts/Utilities/Definitions.hpp"

namespace Acts {
/// @brief NavigationOptions for navigation through the detector
template <typename object_t>
struct NavigationOptions {
  /// The navigation direction
  NavigationDirection navDir = forward;
  /// Start object to be excluded if found
  const object_t* startObject = nullptr;
  /// End object to be excluded & sets path limit
  const object_t* endObject = nullptr;
  /// A given path limit
  double pathLimit = std::numeric_limits<double>::max();
  /// A potential overstep limit
  double overstepLimit = 0.;
  /// A potential opening angle
  double openingAngle = 0.;
  /// A boundary check prescription for the search
  BoundaryCheck boundaryCheck = true;
  /// What to look for : sensitives ?
  bool resolveSensitive = true;
  /// What to look for : material ?
  bool resolveMaterial = true;
  /// What to look for : everything ?
  bool resolveEverything = true;
};
}  // namespace Acts