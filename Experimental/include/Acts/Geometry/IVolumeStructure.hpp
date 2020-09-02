// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <vector>
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/NavigationOptions.hpp"
#include "Acts/Surfaces/BoundaryCheck.hpp"
#include "Acts/Utilities/Intersection.hpp"

namespace Acts {

class Surface;
class VolumeBounds;

/// Surface intersection
using SurfaceIntersection = ObjectIntersection<Surface>;
/// Surface candidates
using SurfaceCandidates = std::vector<SurfaceIntersection>;
/// Surface options
using SurfaceOptions = NavigationOptions<Surface>;

/// @brief base class for Internal Detector Volume description
///
/// This class holds the information about surfaces and additional
/// substructure of a DetectorVolume. The VolumeBounds provided by
/// the structure are used to bound the DetectorVolume.
///
class IVolumeStructure {
 public:
  IVolumeStructure() = default;
  virtual ~IVolumeStructure() = default;

  /// Return the bounds as a plain pointer
  /// @note these bounds will be assigned to the DetectorVolume
  virtual const VolumeBounds& volumeBounds() const = 0;

  /// Return const access to all contained surfaces
  virtual const std::vector<const Surface*>& containedSurfaces() const = 0;

  /// Return the surface candidates int this structure
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The position for searching
  /// @param direction The direction for searching (singed with navigation)
  /// @param options The navigation optsions
  ///
  /// @return the Candidate surfaces for this volume structure
  virtual SurfaceCandidates surfaceCandidates(
      const GeometryContext& gctx, const Vector3D& position,
      const Vector3D& direction, const SurfaceOptions& options) const = 0;
};

}  // namespace Acts