// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

inline const VolumeBounds& DetectorVolume::volumeBounds() const {
  return (*m_volumeBounds);
}

inline const Transform3D& DetectorVolume::transform(
    const GeometryContext& /*ignored*/) const {
  return m_transform;
}

inline const Transform3D& DetectorVolume::inverseTransform(
    const GeometryContext& /*ignored*/) const {
  return m_inverseTransform;
}

inline const Vector3D DetectorVolume::center(
    const GeometryContext& gctx) const {
  auto tMatrix = transform(gctx).matrix();
  return Vector3D(tMatrix(0, 3), tMatrix(1, 3), tMatrix(2, 3));
}

inline bool DetectorVolume::inside(const GeometryContext& gctx,
                                   const Vector3D& position,
                                   double tolerance) const {
  return m_volumeBounds->inside(inverseTransform(gctx) * position, tolerance);
}

inline const DetectorVolume* DetectorVolume::portalVolume(
    const GeometryContext& gctx, const Vector3D& position) const {
  if (m_containerStructure != nullptr) {
    auto sVolume = m_containerStructure->detectorVolume(gctx, position);
    if (sVolume != nullptr) {
      return sVolume->portalVolume(gctx, position);
    }
  }
  return this;
}

inline const ContainerStructure* DetectorVolume::containerStructure() const {
  return m_containerStructure.get();
}

inline const IVolumeStructure* DetectorVolume::volumeStructure() const {
  return m_volumeStructure.get();
}

inline const IVolumeMaterial* DetectorVolume::volumeMaterial() const {
  return m_volumeMaterial.get();
}

inline const DetectorVolume::BoundaryPortalPtrVector&
DetectorVolume::boundaryPortals() const {
  return m_boundaryPortals;
}

inline const std::string& DetectorVolume::volumeName() const {
  return m_volumeName;
}

DetectorVolume::BoundaryPortalCandidates
DetectorVolume::boundaryPortalCandidates(const GeometryContext& gctx,
                                         const Vector3D& position,
                                         const Vector3D& direction,
                                         const BoundaryOptions& options) const {
  // Loop over boundary portals and calculate the intersection
  auto excludeObject = options.startObject;
  BoundaryPortalCandidates bIntersections;

  // The signed direction: solution (except overstepping) is positive
  auto sDirection = options.navDir * direction;

  // The Limits: current, path & overstepping
  double pLimit = options.pathLimit;
  double oLimit = options.overstepLimit;

  // Helper function to test intersection
  auto checkIntersection =
      [&](SurfaceIntersection& sIntersection,
          const BoundaryPortal* bSurface) -> BoundaryPortalIntersection {
    // Avoid doing anything if that's a rotten apple already
    if (!sIntersection) {
      return BoundaryPortalIntersection();
    }

    double cLimit = sIntersection.intersection.pathLength;
    // Check if the surface is within limit
    bool withinLimit =
        (cLimit > oLimit and
         cLimit * cLimit <= pLimit * pLimit + s_onSurfaceTolerance);
    if (withinLimit) {
      sIntersection.intersection.pathLength *=
          std::copysign(1., options.navDir);
      return BoundaryPortalIntersection(sIntersection.intersection, bSurface,
                                        sIntersection.object);
    }
    // Check the alternative
    if (sIntersection.alternative) {
      // Test the alternative
      cLimit = sIntersection.alternative.pathLength;
      withinLimit = (cLimit > oLimit and
                     cLimit * cLimit <= pLimit * pLimit + s_onSurfaceTolerance);
      if (sIntersection.alternative and withinLimit) {
        sIntersection.alternative.pathLength *=
            std::copysign(1., options.navDir);
        return BoundaryPortalIntersection(sIntersection.alternative, bSurface,
                                          sIntersection.object);
      }
    }
    // Return an invalid one
    return BoundaryPortalIntersection();
  };

  /// Helper function to process boundary surfaces
  auto processBoundaryPortals =
      [&](const BoundaryPortalPtrVector& bsPortal) -> void {
    // Loop over the boundary surfaces
    for (auto& bsIter : bsPortal) {
      // Exclude the boundary where you are on
      if (excludeObject != bsIter.get()) {
        auto bCandidate = bsIter->surfaceRepresentation().intersect(
            gctx, position, sDirection, options.boundaryCheck);
        // Intersect and continue
        auto bIntersection = checkIntersection(bCandidate, bsIter.get());
        if (bIntersection) {
          bIntersections.push_back(bIntersection);
        }
      }
    }
  };

  // Process the boundaries of the current volume
  auto& bPortals = boundaryPortals();
  processBoundaryPortals(bPortals);

  // Sort them accordingly to the navigation direction
  if (options.navDir == forward) {
    std::sort(bIntersections.begin(), bIntersections.end());
  } else {
    std::sort(bIntersections.begin(), bIntersections.end(), std::greater<>());
  }
  return bIntersections;
}

DetectorVolume::SurfaceCandidates DetectorVolume::surfaceCandidates(
    const GeometryContext& gctx, const Vector3D& position,
    const Vector3D& direction, const SurfaceOptions& options) const {
  if (m_volumeStructure) {
    return m_volumeStructure->surfaceCandidates(gctx, position, direction,
                                                options);
  }
  return {};
}
