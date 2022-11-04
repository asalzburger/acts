// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Experimental/DetectorVolume.hpp"

#include "Acts/Experimental/NavigationState.hpp"
#include "Acts/Experimental/Portal.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <assert.h>

Acts::Experimental::DetectorVolume::DetectorVolume(
    const GeometryContext&, const std::string& name,
    const Transform3& transform, std::unique_ptr<VolumeBounds> bounds,
    const std::vector<std::shared_ptr<Surface>>& surfaces,
    const std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    ManagedNavigationStateUpdator&& navStateUpdator)
    : m_name(name),
      m_transform(transform),
      m_bounds(std::move(bounds)),
      m_navigationStateUpdator(std::move(navStateUpdator)),
      m_volumeMaterial(nullptr) {
  if (m_bounds == nullptr) {
    throw std::invalid_argument(
        "DetectorVolume: construction with nullptr bounds.");
  }
  if (not navStateUpdator.delegate.connected()) {
    throw std::invalid_argument(
        "DetectorVolume: navigation state updator delegate is not connected.");
  }

  m_surfaces = ObjectStore<std::shared_ptr<Surface>>(surfaces);
  m_volumes = ObjectStore<std::shared_ptr<DetectorVolume>>(volumes);

  assert(checkContainment(gctx) and "Objects are not contained by volume.");
}

Acts::Experimental::DetectorVolume::DetectorVolume(
    const GeometryContext&, const std::string& name,
    const Transform3& transform, std::unique_ptr<VolumeBounds> bounds,
    ManagedNavigationStateUpdator&& navStateUpdator)
    : m_name(name),
      m_transform(transform),
      m_bounds(std::move(bounds)),
      m_navigationStateUpdator(std::move(navStateUpdator)),
      m_volumeMaterial(nullptr) {
  if (m_bounds == nullptr) {
    throw std::invalid_argument(
        "DetectorVolume: construction with nullptr bounds.");
  }
  if (not navStateUpdator.delegate.connected()) {
    throw std::invalid_argument(
        "DetectorVolume: navigation state updator delegate is not connected.");
  }
}

void Acts::Experimental::DetectorVolume::updatePortal(
    std::shared_ptr<Portal> portal, unsigned int pIndex) {
  if (pIndex >= m_portals.internal.size()) {
    throw std::invalid_argument(
        "DetectorVolume: trying to update a portal that does not exist.");
  }
  m_portals.internal[pIndex] = portal;
  m_portals = ObjectStore<std::shared_ptr<Portal>>(m_portals.internal);
}

void Acts::Experimental::DetectorVolume::construct(
    const GeometryContext& gctx, const PortalGenerator& portalGenerator) {
  // Create portals with the given generator
  auto portalSurfaces =
      portalGenerator(transform(gctx), *(m_bounds.get()), getSharedPtr());
  m_portals = ObjectStore<std::shared_ptr<Portal>>(portalSurfaces);
}

std::shared_ptr<Acts::Experimental::DetectorVolume>
Acts::Experimental::DetectorVolume::getSharedPtr() {
  return shared_from_this();
}

std::shared_ptr<const Acts::Experimental::DetectorVolume>
Acts::Experimental::DetectorVolume::getSharedPtr() const {
  return shared_from_this();
}

bool Acts::Experimental::DetectorVolume::inside(const GeometryContext& gctx,
                                                const Vector3& position,
                                                bool excludeInserts) const {
  Vector3 posInVolFrame((transform(gctx).inverse()) * position);
  if (not volumeBounds().inside(posInVolFrame)) {
    return false;
  }
  if (not excludeInserts or m_volumes.external.empty()) {
    return true;
  }
  // Check exclusion through subvolume
  for (const auto v : volumes()) {
    if (v->inside(gctx, position)) {
      return false;
    }
  }
  return true;
}

void Acts::Experimental::DetectorVolume::updateNavigationStatus(
    NavigationState& nState, const GeometryContext& gctx,
    const Vector3& position, const Vector3& direction, ActsScalar absMomentum,
    ActsScalar charge) const {
  // This should not happen too often as the external volume
  // finders should direct into the subvolumes already
  if (m_volumes.external.size()) {
    for (const auto v : volumes()) {
      if (v->inside(gctx, position)) {
        v->updateNavigationStatus(nState, gctx, position, direction,
                                  absMomentum, charge);
        return;
      }
    }
  }
  // The state updator of the volume itself
  m_navigationStateUpdator.delegate(nState, *this, gctx, position, direction,
                                    absMomentum, charge);
  nState.currentVolume = this;
  nState.surfaceCandidate = nState.surfaceCandidates.begin();
  return;
}

void Acts::Experimental::DetectorVolume::updateNavigationStateUpator(
    ManagedNavigationStateUpdator&& navStateUpdator,
    const std::vector<std::shared_ptr<Surface>>& surfaces,
    const std::vector<std::shared_ptr<DetectorVolume>>& volumes) {

  m_navigationStateUpdator = std::move(navStateUpdator);
  m_surfaces = ObjectStore<std::shared_ptr<Surface>>(surfaces);
  m_volumes = ObjectStore<std::shared_ptr<DetectorVolume>>(volumes);

  assert(checkContainment(gctx) and "Objects are not contained by volume.");
}

void Acts::Experimental::DetectorVolume::resize(
    const GeometryContext& gctx, std::unique_ptr<VolumeBounds> rBounds,
    const PortalGenerator& portalGenerator) {
  if (rBounds == nullptr or rBounds->type() != m_bounds->type()) {
    throw std::invalid_argument(
        "DetectorVolume: wrong bound type provided for resize(..) call");
  }
  m_bounds = std::move(rBounds);
  construct(gctx, portalGenerator);
  assert(checkContainment(gctx, 72u) and
         "Objects are not contained by volume.");
}

Acts::Extent Acts::Experimental::DetectorVolume::extent(
    const GeometryContext& gctx, size_t nseg) const {
  Extent volumeExtent;
  for (const auto* p : portals()) {
    volumeExtent.extend(
        p->surface().polyhedronRepresentation(gctx, nseg).extent());
  }
  return volumeExtent;
}

bool Acts::Experimental::DetectorVolume::checkContainment(
    const GeometryContext& gctx, size_t nseg) const {
  // Create the volume extent
  auto volumeExtent = extent(gctx, nseg);
  // Check surfaces
  for (const auto* s : surfaces()) {
    auto sExtent = s->polyhedronRepresentation(gctx, nseg).extent();
    if (not volumeExtent.contains(sExtent)) {
      return false;
    }
  }
  // Check volumes
  for (const auto* v : volumes()) {
    auto vExtent = v->extent(gctx, nseg);
    if (not volumeExtent.contains(vExtent)) {
      return false;
    }
  }
  // All contained
  return true;
}

void Acts::Experimental::DetectorVolume::lock(
    const GeometryIdentifier& geometryId) {
  m_geometryId = geometryId;

  // Assign the boundary Identifier
  GeometryIdentifier portalId = geometryId;
  for (auto [i, p] : enumerate(m_portals.internal)) {
    portalId.setBoundary(i + 1);
    p->assignGeometryId(portalId);
  }

  // Assign the sensitive/surface Identifier
  GeometryIdentifier sensitiveId = geometryId;
  /// @todo add passive count
  for (auto [i, s] : enumerate(m_surfaces.internal)) {
    sensitiveId.setSensitive(i + 1);
    s->assignGeometryId(sensitiveId);
  }

  // Check if it is a container or detector volume
  if (not m_volumes.internal.empty()) {
    // Detection if any of the volume has sub surfaces
    bool detectorVolume = false;
    for (auto v : volumes()) {
      // Would in principle qualify
      if (not v->surfaces().empty()) {
        detectorVolume = true;
        break;
      }
    }
    // Cross-check if no container is present
    for (auto v : volumes()) {
      // Pure detector volume is vetoed
      if (not v->volumes().empty()) {
        detectorVolume = false;
        break;
      }
    }

    // Assign the volume Identifier (recursive step down)
    for (auto [i, v] : enumerate(m_volumes.internal)) {
      GeometryIdentifier volumeId = geometryId;
      if (detectorVolume) {
        volumeId.setLayer(i + 1);
      } else {
        volumeId.setVolume(volumeId.volume() + i + 1);
      }
      v->lock(volumeId);
    }
  }
}
