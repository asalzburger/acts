// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>
#include <vector>
#include "Acts/Geometry/IVolumeStructure.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Surfaces/BoundaryCheck.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinnedArrayXD.hpp"

namespace Acts {

class Surface;

/// @brief class to describe an internal layer structure
/// of a DetectorVolume.
///
/// The layer can have a variable number of internal surfaces
/// (sensitive and non-sensitive) which are held by this struct
/// and provided through the DetectorVolume to the Navigator
///
/// This class also holds a VolumeBounds object which defines
/// the size of the volume surrounding volume.
class LayerStructure : public IVolumeStructure {
 public:
  LayerStructure() = delete;

  /// Constructor with arguments - for single-surface layer
  /// @param volumeBounds The bounds of the DetectorVolume
  /// @param surfaces The contained surface (to keep track of ownership)
  LayerStructure(std::unique_ptr<VolumeBounds> volumeBounds,
                 std::shared_ptr<Surface> surface);

  /// Constructor with arguments - for simple layer structure
  /// @param volumeBounds The bounds of the DetectorVolume
  /// @param surfaceArray The surface arrays for navigation
  /// @param surfaces The containes surfaces (to keep track of ownership)
  LayerStructure(std::unique_ptr<VolumeBounds> volumeBounds,
                 SurfaceArray&& surfaceArray,
                 std::vector<std::shared_ptr<Surface>> surfaces);

  /// Constructor with arguments - for complicated layer structure
  /// @param volumeBounds The bounds of the DetectorVolume
  /// @param surfaceArrays The surface arrays for navigation
  /// @param surfaces The surface ownership
  LayerStructure(
      std::unique_ptr<VolumeBounds> volumeBounds,
      BinnedArrayXD<SurfaceArray>&& surfaceArrays,
      std::vector<std::shared_ptr<Surface>> surfaces);

  ~LayerStructure() = default;

  /// Return the VolumeBounds as a reference
  /// @note these bounds will be assigned to the DetectorVolume
  const VolumeBounds& volumeBounds() const final;

  /// Return const access to all contained surfaces
  const SurfaceVector& containedSurfaces() const final;

  /// Return the surface candidates in this structure
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The position for searching
  /// @param direction The direction for searching (singed with navigation)
  /// @param options The navigation options
  ///
  /// @return the Candidate surfaces for this volume structure
  SurfaceCandidates surfaceCandidates(
      const GeometryContext& gctx, const Vector3D& position,
      const Vector3D& direction, const SurfaceOptions& options) const final;

 private:
  /// Consistency check for constructors
  void checkConsistency() const noexcept(false);
  /// The volume bounds describing this layer
  std::unique_ptr<VolumeBounds> m_volumeBounds;
  /// Ths Surface array for ordered surfaces - navigation only
  BinnedArrayXD<SurfaceArray> m_surfaceArrays;
  /// The ownership of these surfaces
  std::vector<std::shared_ptr<Surface>> m_surfaces;
  /// The contained surfaces for const return
  std::vector<const Surface*> m_containedSurfaces;
};

inline void LayerStructure::checkConsistency() const {
  if (m_volumeBounds == nullptr) {
    throw std::domain_error("LayerStructure: must have VolumeBounds.");
  }
}

inline const VolumeBounds& LayerStructure::volumeBounds() const {
  return (*m_volumeBounds.get());
}

inline const std::vector<const Surface*>& LayerStructure::containedSurfaces() const {
  return m_containedSurfaces;
}

inline SurfaceCandidates LayerStructure::surfaceCandidates(
    const GeometryContext& gctx, const Vector3D& position,
    const Vector3D& direction, const SurfaceOptions& options) const {
  // List of valid intersection
  std::vector<SurfaceIntersection> sIntersections;
  std::map<const Surface*, bool> accepted;

  // Reserve a few bins
  sIntersections.reserve(20);

  double pathLimit = options.pathLimit;
  double overstepLimit = options.overstepLimit;
  if (options.endObject) {
    // Intersect the end surface
    // - it is the final one don't use the bounday check at all
    SurfaceIntersection eIntersection = options.endObject->intersect(
        gctx, position, options.navDir * direction, BoundaryCheck(true));
    // Non-valid intersection with the end surface provided at this layer
    // indicates wrong direction or faulty setup
    if (eIntersection) {
      pathLimit = std::min(pathLimit, eIntersection.intersection.pathLength);
    }
  }

  // Check whether to accept the surface
  auto acceptSurface = [&options, &accepted](const Surface& sf,
                                             bool sensitive = false) -> bool {
    // Check for duplicates @todo : check if needed, could omit map then
    if (accepted.find(&sf) != accepted.end()) {
      return false;
    }
    // Surface is sensitive and you're asked to resolve
    if (sensitive && options.resolveSensitive) {
      return true;
    }
    // Next option: it's a material surface and you want to have it
    if (options.resolveMaterial && sf.surfaceMaterial()) {
      return true;
    }
    // Last option: resovle everything
    return options.resolveEverything;
  };

  // Check and fill the surface
  auto processSurface = [&](const Surface& sf, bool sensitive = false) {
    // veto if it's start or end surface
    if (options.startObject == &sf || options.endObject == &sf) {
      return;
    }
    // Veto if it doesn't fit the prescription
    if (!acceptSurface(sf, sensitive)) {
      return;
    }
    // Intersect & check
    SurfaceIntersection sfi = sf.intersect(
        gctx, position, options.navDir * direction, options.boundaryCheck);
    double sifPath = sfi.intersection.pathLength;
    if (sfi && sifPath > overstepLimit &&
        sifPath * sifPath <= pathLimit * pathLimit) {
      // Assign the right sign
      sfi.intersection.pathLength *= std::copysign(1., options.navDir);
      sIntersections.push_back(sfi);
      accepted[&sf] = true;
    }
    return;
  };

  const auto& sArray = m_surfaceArrays.object(position);
  const auto& surfaces = sArray.neighbors(position);
  std::for_each(surfaces.begin(), surfaces.end(),
                  [&](const auto& sf) { processSurface(*sf); });

  // sort according to the path length
  if (options.navDir == forward) {
    std::sort(sIntersections.begin(), sIntersections.end());
  } else {
    std::sort(sIntersections.begin(), sIntersections.end(), std::greater<>());
  }
  return sIntersections;
}

}  // namespace Acts