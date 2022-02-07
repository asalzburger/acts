// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Experimental/Portal.hpp"

#include "Acts/Experimental/DetectorVolume.hpp"
#include "Acts/Surfaces/Surface.hpp"

Acts::Portal::Portal(std::shared_ptr<Surface> surface)
    : m_surface(std::move(surface)) {}

std::shared_ptr<Acts::Portal> Acts::Portal::getSharedPtr() {
  return shared_from_this();
}

std::shared_ptr<const Acts::Portal> Acts::Portal::getSharedPtr() const {
  return shared_from_this();
}

void Acts::Portal::assignSurfaceMaterial(
    std::shared_ptr<const ISurfaceMaterial> material) {
  return m_surface->assignSurfaceMaterial(material);
}

Acts::PortalIntersection Acts::Portal::intersect(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction) const {
  // The surface intersection
  SurfaceIntersection sIntersection =
      m_surface->intersect(gctx, position, direction, true);
  // Return it as a portal intersection
  PortalIntersection pIntersection{sIntersection.intersection, this,
                                   m_surface.get()};
  pIntersection.alternative = sIntersection.alternative;
  return pIntersection;
}

Acts::DetectorEnvironment Acts::Portal::next(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction, ActsScalar absMomentum, ActsScalar charge,
    const BoundaryCheck& bCheck, const std::array<ActsScalar, 2>& pathRange,
    bool provideAll) const {
  // Chose which side w.r.t. to the normal the portal jump happens
  ActsScalar normalProjection =
      m_surface->normal(gctx, position).dot(direction);

  // Return along
  if (normalProjection > 0.) {
    return m_alongNormal.connected()
               ? m_alongNormal(gctx, *this, position, direction, absMomentum,
                               charge, bCheck, pathRange, provideAll)
               : DetectorEnvironment{};
  }
  // ... or opposite
  return m_oppositeNormal.connected()
             ? m_oppositeNormal(gctx, *this, position, direction, absMomentum,
                                charge, bCheck, pathRange, provideAll)
             : DetectorEnvironment{};
}

void Acts::Portal::assignGeometryId(const GeometryIdentifier& geometryId) {
  m_surface->assignGeometryId(geometryId);
}

void Acts::Portal::updatePortalLink(PortalLink&& portalLink,
                                    NavigationDirection nDir,
                                    std::shared_ptr<void> portalLinkImpl) {
  if (nDir == forward) {
    m_alongNormal = std::move(portalLink);
  } else {
    m_oppositeNormal = std::move(portalLink);
  }
  if (portalLinkImpl != nullptr) {
    m_linkImplStore.insert(portalLinkImpl);
  }
}

std::shared_ptr<Acts::Portal> Acts::Portal::connect(Portal& rhs) const {
  if (rhs.m_alongNormal.connected() and rhs.m_oppositeNormal.connected()) {
    throw std::invalid_argument(
        "\n *** Portal: trying to connect an already fully connected portal.");
  }
  if (rhs.m_alongNormal.connected() and m_oppositeNormal.connected()) {
    rhs.m_oppositeNormal = m_oppositeNormal;
  } else if (rhs.m_oppositeNormal.connected() and m_alongNormal.connected()) {
    rhs.m_alongNormal = m_alongNormal;
  } else {
    throw std::invalid_argument(
        "\n *** Portal: connect() call would leave incomplete portal.");
  }
  return rhs.getSharedPtr();
}

Acts::PortalCandidates Acts::Portal::portalCandidates(
    const GeometryContext& gctx, const std::vector<const Portal*>& portals,
    const Vector3& position, const Vector3& direction,
    const std::array<ActsScalar, 2>& pathRange) {
  // The portal intersections
  PortalCandidates pIntersections;
  // Get all the portals
  pIntersections.reserve(portals.size());
  // Loop over portals an intersect
  for (const auto& p : portals) {
    // Get the intersection
    auto pIntersection = p->intersect(gctx, position, direction);
    // Re-order if necessary
    if (pIntersection.intersection.pathLength + s_onSurfaceTolerance <
            pathRange[0] and
        pIntersection.alternative.pathLength + s_onSurfaceTolerance >
            pathRange[0] and
        pIntersection.alternative.status >= Intersection3D::Status::reachable) {
      // Let's swap the solutions
      pIntersection.swapSolutions();
    }
    // Exclude on-portal solution
    if (std::abs(pIntersection.intersection.pathLength) <
        s_onSurfaceTolerance) {
      continue;
    }
    pIntersections.push_back(pIntersection);
  }
  // Sort and non-allowed solutions to the end
  std::sort(
      pIntersections.begin(), pIntersections.end(),
      [&](const auto& a, const auto& b) {
        if (a.intersection.pathLength + s_onSurfaceTolerance < pathRange[0]) {
          return false;
        } else if (b.intersection.pathLength + s_onSurfaceTolerance <
                   pathRange[0]) {
          return true;
        }
        return a.intersection.pathLength < b.intersection.pathLength;
      });
  // Return the sorted solutions
  return pIntersections;
};
