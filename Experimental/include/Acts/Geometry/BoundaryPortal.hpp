// This file is part of the Acts project.
//
// Copyright (C) 2016-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <array>
#include <memory>
#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinnedArrayXD.hpp"
#include "Acts/Utilities/Definitions.hpp"

namespace Acts {

class Surface;

/// Method to attach another portal to one portal, the surface
/// of the one surface is kept and other is eventually set to one
///
/// @param one the first boundary portal
/// @param other the second boundary portal
template <typename boundary_portal_t>
void attachPortal(std::shared_ptr< boundary_portal_t >& one, 
              std::shared_ptr< boundary_portal_t >& other){

  // Unify the surface to be the same
  other->m_surface = one->m_surface;
  // Keep the volume attachment information of the other
  auto otherAttachedVolumes = other->m_attachedVolumes[0].empty() ? 
    other->m_attachedVolumes[1]  : other->m_attachedVolumes[0];
  if (one->m_attachedVolumes[0].empty()) {
    one->m_attachedVolumes[0] = otherAttachedVolumes;
  } else {
    one->m_attachedVolumes[1] = otherAttachedVolumes;
  }
  other = one;
}

/// Method to stitch a new portal to an exising compound
///
/// @param one the first boundary portal
/// @param other the second boundary portal
///
/// @note does nothing if stitching does not work
template <typename boundary_portal_t>
void stitchPortal(std::shared_ptr< boundary_portal_t >& one, 
                  std::shared_ptr< boundary_portal_t >& other){

  const auto& oneSurface = one->surfaceRepresentation();
  const auto& otherSurface = other->surfaceRepresentation();
  auto stitchedSurface = oneSurface.stitch(GeometryContext(), otherSurface);
  if (stitchedSurface != nullptr){
    one->m_surface = stitchedSurface;
    other = one;
    // @TODO stitch arrays
  }
}

/// @class BoundaryPortal
///
/// The templated boundary portal class connects volumes via the transient
/// boundary portal mechanism. The volumes are attached with respect to the
/// portal surface normal vector.
///
/// @note The boundary portals only hold const raw pointers, as they need fast
/// navigaiton and are not involved in geometry ownership.
///
/// @tparam volume_t The template parameter for the volume type
///
template <typename volume_t>
class BoundaryPortal {
  friend volume_t;

  friend void attachPortal< BoundaryPortal<volume_t> >(
      std::shared_ptr< BoundaryPortal<volume_t> >& one, 
              std::shared_ptr< BoundaryPortal<volume_t> >& other);

  friend void stitchPortal< BoundaryPortal<volume_t> >(
      std::shared_ptr< BoundaryPortal<volume_t> >& one, 
              std::shared_ptr< BoundaryPortal<volume_t> >& other);


  using VolumeArray = BinnedArrayXD<const volume_t*>;

 public:
  BoundaryPortal() = delete;

  /// Constructor for a Boundary with exact two Volumes attached to it
  /// - usually used in a volume constructor
  ///
  /// @param surface The surface representing this boundary
  /// @param opposite The opposite volume the bounday surface points to
  /// @param along The along volume the boundary surface points to
  BoundaryPortal(std::shared_ptr<Surface> surface, const volume_t* opposite,
                 const volume_t* along)
      : m_surface(std::move(surface)),
        m_attachedVolumes({VolumeArray(opposite), VolumeArray(along)}) {}

  /// Constructor for a Boundary with exact multiple Volumes attached to it
  /// - usually used in a volume constructor
  ///
  /// @param surface The unqiue surface the boundary represents
  /// @param oppositeArray The opposite volume array the bounday surface points
  /// to
  /// @param alongArray The along volume array the boundary surface
  /// points to
  BoundaryPortal(std::shared_ptr<Surface> surface, VolumeArray oppositeArray,
                 VolumeArray alongArray)
      : m_surface(std::move(surface)),
        m_attachedVolumes({oppositeArray, alongArray}) {}

  /// Get the next Volume depending on GlobalPosition, GlobalMomentum, dir on
  /// the TrackParameters and the requested direction
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param pos The global position on surface
  /// @param mom The direction on the surface
  /// @param ndir is an aditional direction corrective
  ///
  /// @return The attached volume at that position
  virtual const volume_t* nextVolume(const GeometryContext& gctx,
                                     const Vector3D& pos, const Vector3D& mom,
                                     NavigationDirection ndir) const;

  /// templated onBoundary method
  ///
  /// @tparam parameters_t are the parameters to be checked
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param pars The parameters used for this call
  template <class parameters_t>
  bool onBoundary(const GeometryContext& gctx, const parameters_t& pars) const {
    return surfaceRepresentation().isOnSurface(gctx, pars);
  }

  /// The Surface Representation of this
  virtual const Surface& surfaceRepresentation() const;

  /// Virtual Destructor
  virtual ~BoundaryPortal() = default;

  /// Helper method: attach a Volume to this BoundaryPortal
  /// this is done during the geometry construction.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param volume The volume to be attached
  /// @param ndir The navigation direction
  void attachVolume(const volume_t* volume, NavigationDirection ndir);

  /// Helper method: attach a Volume to this BoundaryPortal
  /// this is done during the geometry construction.
  ///
  /// @param volumes The volume array to be attached
  /// @param ndir The navigation direction
  void attachVolumeArray(VolumeArray volumes, NavigationDirection ndir);

 protected:
  /// The represented surface by this
  std::shared_ptr<Surface> m_surface;
  /// The attached volumes
  std::array<VolumeArray, 2> m_attachedVolumes;
};


template <typename volume_t>
const Surface& BoundaryPortal<volume_t>::surfaceRepresentation() const {
  return (*(m_surface.get()));
}


template <typename volume_t>
void BoundaryPortal<volume_t>::attachVolume(const volume_t* volume,
                                            NavigationDirection ndir) {
  size_t acc = (ndir == backward) ? 0 : 1;
  m_attachedVolumes[acc] = VolumeArray(volume);
}

template <typename volume_t>
void BoundaryPortal<volume_t>::attachVolumeArray(VolumeArray volumes,
                                                 NavigationDirection ndir) {
  size_t acc = (ndir == backward) ? 0 : 1;
  m_attachedVolumes[acc] = volumes;
}

template <typename volume_t>
const volume_t* BoundaryPortal<volume_t>::nextVolume(
    const GeometryContext& gctx, const Vector3D& pos, const Vector3D& mom,
    NavigationDirection ndir) const {
  const volume_t* nextVolume = nullptr;
  // Dot product with the normal vector
  size_t acc =
      surfaceRepresentation().normal(gctx, pos).dot(ndir * mom) > 0. ? 1 : 0;
  return m_attachedVolumes[acc].object(pos);
  ;
}

}  // namespace Acts
