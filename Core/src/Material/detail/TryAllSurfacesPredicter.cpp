// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Material/detail/TryAllSurfacesPredicter.hpp"
#include "Acts/Surfaces/BoundaryCheck.hpp"

std::vector<Acts::SurfaceIntersection>
Acts::detail::TryAllSurfacesPredicter::operator()(
    const GeometryContext &gctx, const MagneticFieldContext & /*ignored*/,
    const Vector3 &position, const Vector3 &direction) {
  // Prepare the return object
  Prediction prediction;
  prediction.reserve(nReserve);

  // Loop over all surfaces
  for (const auto &surface : surfaces) {
    // Get the intersection
    auto intersectionCandidate =
        surface->intersect(gctx, position, direction, BoundaryCheck(true))
            .closestForward();
    if (intersectionCandidate.status() >= IntersectionStatus::reachable &&
        intersectionCandidate.pathLength() > 0.) {
      // Add the intersection to the prediction
      prediction.push_back(intersectionCandidate);
    }
  }
  std::sort(prediction.begin(), prediction.end(), SurfaceIntersection::forwardOrder);
  // Return the prediction
  return prediction;
}
