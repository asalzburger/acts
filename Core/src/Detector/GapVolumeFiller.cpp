// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Detector/GapVolumeFiller.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Navigation/SurfaceCandidatesUpdaters.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <map>

void Acts::Experimental::GapVolumeFiller::apply(const GeometryContext& gctx,
                                                Detector& detector) const {
  auto mutableVolumes = detector.volumePtrs();

  std::map<std::shared_ptr<DetectorVolume>,
           std::vector<std::shared_ptr<Surface>>>
      volumeSurfacesAssignments;

  for (auto surface : m_cfg.surfaces) {
    Vector3 searchPosition = surface->center(gctx);
    // Refine the search position in case you have cylinder or disc
    if (surface->type() == RegularSurface::SurfaceType::Cylinder) {
      auto bValues = surface->bounds().values();
      ActsScalar r = bValues[CylinderBounds::eR];
      ActsScalar avgPhi = bValues[CylinderBounds::eAveragePhi];
      Vector3 locSearchPosition(r * std::cos(avgPhi), r * std::sin(avgPhi), 0.);
      searchPosition = surface->transform(gctx) * locSearchPosition;
    } else if (surface->type() == RegularSurface::SurfaceType::Disc) {
      auto bValues = surface->bounds().values();
      ActsScalar r =
          0.5 * (bValues[RadialBounds::eMinR] + bValues[RadialBounds::eMaxR]);
      ActsScalar avgPhi = bValues[RadialBounds::eAveragePhi];
      Vector3 locSearchPosition(r * std::cos(avgPhi), r * std::sin(avgPhi), 0.);
      searchPosition = surface->transform(gctx) * locSearchPosition;
    }

    // Find the volume that contains the search position
    const DetectorVolume* volume =
        detector.findDetectorVolume(gctx, searchPosition);
    auto mutableVolume =
        std::find_if(mutableVolumes.begin(), mutableVolumes.end(),
                     [&](auto& v) -> bool { return (v.get() == volume); });
    if (mutableVolume == mutableVolumes.end()) {
      ACTS_WARNING("Volume not found for surface!");
      continue;
    }

    std::shared_ptr<DetectorVolume> volumePtr = *mutableVolume;
    // Update the volume with the surface
    if (!volumePtr->surfaces().empty()) {
      ACTS_WARNING("Volume "
                   << volume->name()
                   << " already contains a surface, updating an existing local "
                      "navigation delegate is not supported.");
    } else {
      // Make sure it exists
      if (volumeSurfacesAssignments.find(volumePtr) ==
          volumeSurfacesAssignments.end()) {
        volumeSurfacesAssignments[volumePtr] =
            std::vector<std::shared_ptr<Surface>>();
      }
      volumeSurfacesAssignments[volumePtr].push_back(surface);
    }
  }

  // Finally assign the things
  for (auto& [volume, surfaces] : volumeSurfacesAssignments) {
    // Assign the geoIDs
    for (auto [is, s] : enumerate(surfaces)) {
      auto surfaceId =
          GeometryIdentifier(volume->geometryId()).setPassive(is + 1);
      s->assignGeometryId(surfaceId);
    }

    volume->assignSurfaceCandidatesUpdater(tryAllPortalsAndSurfaces(),
                                           surfaces);
  }
}