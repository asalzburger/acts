// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Intersection.hpp"

#include <vector>
#include <boost/container/small_vector.hpp>

/// @note this is foreseen for the 'Geometry' module

namespace Acts {

class DetectorVolume;
class Portal;
class Surface;

using SurfaceIntersection = ObjectIntersection<Surface>;
using PortalIntersection = ObjectIntersection<Portal, Surface>;

using SurfaceCandidates = boost::container::small_vector<SurfaceIntersection, 8>;
using PortalCandidates = boost::container::small_vector<PortalIntersection, 8>;


/// A pure navigation struct, that describes the current
/// environment, it is provided/updated by the portal at
/// entry into into detector volume.
struct DetectorEnvironment {
  /// The status of the environment
  enum Status : unsigned int {
    eUninitialized = 0,
    eTowardsSurface = 1,
    eOnSurface = 2,
    eTowardsPortal = 3,
    eOnPortal = 4
  };

  /// The current volume in processing
  const DetectorVolume* currentVolume = nullptr;

  /// The current surface, i.e the track is on surface
  const Surface* currentSurface = nullptr;

  /// That are the candidate surfaces to process
  SurfaceCandidates surfaces = {};
  SurfaceCandidates::iterator surfaceCandidate = surfaces.end();

  /// That are the portals for leaving that environment
  PortalCandidates portals = {};
  PortalCandidates::iterator portalCandidate = portals.end();

  /// Indicate the status of this environment
  Status status = eUninitialized;
};

}  // namespace Acts
