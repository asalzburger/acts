// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Experimental/DetectorEnvironment.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/BoundaryCheck.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Intersection.hpp"

#include <array>
#include <limits>
#include <memory>
#include <set>
#include <vector>

/// @note this file is foreseen for the 'Geometry' module & replace BoundarySurfaceT

namespace Acts {

class ISurfaceMaterial;
class DetectorVolume;
class Portal;
class Surface;

/// Definition of portal link via the Delegate<> schema, it uses
/// the surface link and volume link to establish the detector environment
///
/// @param gctx is the current geometry context
/// @param portal the portal at this request
/// @param position is the current position on (not checked) the portal
/// @param direction is the current direction at that portal
/// @param absMomentum is the absolute mommentum value
/// @param charge is the charge of the particle
/// @param bCheck is the surface boundary check
/// @param pathRange is the min/max path range restriction
/// @param provideAll is a flag to switch on test-all navigation
///
/// @return a new detector environment
using PortalLink = Delegate<DetectorEnvironment(
    const GeometryContext& gctx, const Portal& portal, const Vector3& position,
    const Vector3& direction, ActsScalar absMomentum, ActsScalar charge,
    const BoundaryCheck& bCheck, const std::array<ActsScalar, 2>& pathRange,
    bool provideAll)>;

/// A portal between the detector volumes
///
/// It has a Surface representation for navigation and propagation
/// and guides into the next volumes.
///
/// The surface can also carry material to allow mapping onto
/// portal positions.
///
class Portal : public std::enable_shared_from_this<Portal> {
 protected:
  /// Constructor from surface
  ///
  /// @param surface is the representing surface
  Portal(std::shared_ptr<Surface> surface);

 public:
  /// Declare the DetectorVolume friend for portal setting
  friend class DetectorVolume;

  /// Factory for producing memory managed instances of Surface.
  /// Will forward all parameters and will attempt to find a suitable
  /// constructor.
  template <typename... Args>
  static std::shared_ptr<Portal> makeShared(Args&&... args) {
    return std::shared_ptr<Portal>(new Portal(std::forward<Args>(args)...));
  }

  /// Retrieve a @c std::shared_ptr for this surface (non-const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior (but most likely implemented as a @c bad_weak_ptr
  ///       exception), in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<Portal> getSharedPtr();

  /// Retrieve a @c std::shared_ptr for this surface (const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior, but most likely implemented as a @c bad_weak_ptr
  ///       exception, in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<const Portal> getSharedPtr() const;

  Portal() = delete;
  virtual ~Portal() = default;

  /// Access to the surface representation
  const Surface& surfaceRepresentation() const;

  /// Portal intersection, this forwards to the
  /// Surface intersection
  ///
  /// @param gctx is the current geometry conbtext
  /// @param position is the position at the query
  /// @param direction is the direction at the query
  ///
  /// @return a portal intersection
  PortalIntersection intersect(const GeometryContext& gctx,
                               const Vector3& position,
                               const Vector3& direction) const;

  /// Assign the surface material description
  ///
  /// The material is usually derived in a complicated way and loaded from
  /// a framework given source. As various surfaces may share the same source
  /// this is provided by a shared pointer
  ///
  /// @param material Material description associated to this surface
  void assignSurfaceMaterial(std::shared_ptr<const ISurfaceMaterial> material);

  /// Update the portal link - move semantics, this is
  /// with respect to the normal vector of the surface
  ///
  /// @param portalLink the volume link to be updated
  /// @param nDir the navigation direction
  /// @param portalLinkImple the (optional) link implementation
  void updatePortalLink(PortalLink&& portalLink, NavigationDirection nDir,
                        std::shared_ptr<void> portalLinkImpl = nullptr);

  /// Retrieve the portalLink given the navigation direction
  ///
  /// @param nDir the navigation direction
  ///
  /// @return the portal link as a const object
  const PortalLink& portalLink(NavigationDirection nDir) const;

  /// Conntect the two portals of same size
  ///
  /// This method takes the rhs portal, attaches the missing
  /// portal links from *this* and returns the shared ptr of
  /// rhs.
  ///
  /// @param rhs the other portal with which this one is connected
  ///
  /// The intention here is to overwrite a portal with a connected one
  /// @note this throws an exception if the surface bounds are not
  /// comparable and the @param rhs portal is already fully connected
  std::shared_ptr<Portal> connect(Portal& rhs) const noexcept(false);

  /// Get the next detector environment once you have reached a portal
  ///
  /// @param gctx is the current geometry context object, e.g. alignment
  /// @param position is the global position on surface
  /// @param direction is he direction on the surface
  /// @param absMomentum is the absolute momentum
  /// @param charge is the particle charge
  /// @param bCheck is the boundary check for the surface search
  /// @param pathRange is the min/max path range restriction
  /// @param provideAll is a flag to switch on trial&error navigation
  ///
  /// @return The updated detector environement
  DetectorEnvironment next(const GeometryContext& gctx, const Vector3& position,
                           const Vector3& direction, ActsScalar absMomentum,
                           ActsScalar charge, const BoundaryCheck& bCheck,
                           const std::array<ActsScalar, 2>& pathRange =
                               {0.,
                                std::numeric_limits<ActsScalar>::infinity()},
                           bool provideAll = false) const;

  /// Set the geometry identifier (to the underlying surface)
  ///
  /// @param geometryId the geometry identifier to be assigned
  void assignGeometryId(const GeometryIdentifier& geometryId);

  /// Static Helper function to get the portal candidates from a volume
  ///
  /// @param gctx is the current geometry conbtext
  /// @param portals is the volume for which the portals are intersected
  /// @param position is the position at the query
  /// @param direction is the direction at the query
  /// @param pathRange is the allowed path range for this search
  ///
  /// @note onSurface solutions are ranked last
  static PortalCandidates portalCandidates(
      const GeometryContext& gctx, const std::vector<const Portal*>& portals,
      const Vector3& position, const Vector3& direction,
      const std::array<ActsScalar, 2>& pathRange = {
          0., std::numeric_limits<ActsScalar>::infinity()});

 private:
  /// The surface representation of this portal
  std::shared_ptr<Surface> m_surface;
  /// The entry link along the surface normal direction
  PortalLink m_alongNormal;
  /// The entry link opposite the surfacea normal direction
  PortalLink m_oppositeNormal;
  /// The link implementation store
  std::set<std::shared_ptr<void>> m_linkImplStore;
};

inline const Surface& Portal::surfaceRepresentation() const {
  return *(m_surface.get());
}

inline const PortalLink& Portal::portalLink(NavigationDirection nDir) const {
  if (nDir == forward) {
    return m_alongNormal;
  }
  return m_oppositeNormal;
}

}  // namespace Acts
