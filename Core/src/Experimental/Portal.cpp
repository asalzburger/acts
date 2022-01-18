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
