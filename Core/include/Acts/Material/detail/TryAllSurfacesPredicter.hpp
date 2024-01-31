// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

namespace Acts {
namespace detail {

struct TryAllSurfacesPredicter {
  /// @brief Surface intersection prediction
  using Prediction = std::vector<SurfaceIntersection>;

  /// The vector of surfaces to try
  std::vector<const Surface*> surfaces = {};
  /// The number of candiates for vector reserve
  std::size_t nReserve = 25;

  /// @brief Surface intersection predictor
  /// @param gctx is the geometry context
  /// @param mctx is the magnetic field context
  /// @param position is the position of the track
  /// @param direction is the direction of the track
  Prediction operator()(const GeometryContext &, const MagneticFieldContext &,
                        const Vector3 &position, const Vector3 &direction);
};

}  // namespace detail
}  // namespace Acts
