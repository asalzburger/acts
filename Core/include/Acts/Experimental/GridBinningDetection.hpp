// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Common.hpp"

#include <tuple>
#include <vector>

namespace Acts {

/// This struct performs the detection of the binning structure
/// for given values estimated from geometry parsing
///
struct GridBinningDetection {
  /// A tolerance parameter for the clustering
  ActsScalar clusterTolerance = 0.;
  /// A relative tolerance for equidistant binning
  ActsScalar relEqTolerance = 0.1;
  /// A relative tolerance for equidistant sub binning
  ActsScalar relSubEqTolerance = 0.01;
  /// A boolean to steer angular closure detection
  bool checkPhiWrapping = false;

  /// Call operator for the struct to run binning detection
  ///
  /// @param values [in, out] are the parsed values for the binning check
  /// @note the @param values are being sorted and duplicates removed
  ///
  /// It runs a poor man's clustering algorithm to cluster values and then
  /// perfroms some checking if equidistant binning is available or not
  ///
  /// @note throws exception if misconfigured
  ///
  /// This struct returns { X, {} } for equidistant binning and
  ///                     { X, {a, ...., b} } for non-equidistant binning
  std::tuple<size_t, std::vector<ActsScalar>> operator()(
      std::vector<ActsScalar>& values) noexcept(false);
};
}  // namespace Acts
