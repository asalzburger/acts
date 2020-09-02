// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/DetectorVolume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"

Acts::DetectorVolume::DetectorVolume(
    const Transform3D& transform, std::unique_ptr<ContainerStructure> container,
    const std::string& name)
    : m_transform(transform),
      m_inverseTransform(transform.inverse()),
      m_containerStructure(std::move(container)),
      m_volumeName(name) {
  if (m_containerStructure == nullptr) {
    throw std::domain_error("DetctorVolume: nullptr to ContainerStructure.");
  }
  m_volumeBounds = m_containerStructure->volumeBounds();
  createBoundaryPortals();
}

Acts::DetectorVolume::DetectorVolume(
    const Transform3D& transform,
    std::unique_ptr<IVolumeStructure> volumeStructure,
    std::unique_ptr<IVolumeMaterial> volumeMaterial, const std::string& name)
    : m_transform(transform),
      m_inverseTransform(transform.inverse()),
      m_volumeStructure(std::move(volumeStructure)),
      m_volumeMaterial(std::move(volumeMaterial)),
      m_volumeName(name) {
  if (m_volumeStructure == nullptr) {
    throw std::domain_error("DetctorVolume: nullptr to VolumeStructure.");
  }
  m_volumeBounds = m_volumeStructure->volumeBounds();
  createBoundaryPortals();
}

const Acts::Vector3D Acts::DetectorVolume::binningPosition(
    const GeometryContext& gctx, Acts::BinningValue bValue) const {
  // for most of the binning types it is actually the center,
  // just for R-binning types the
  if (bValue == Acts::binR || bValue == Acts::binRPhi) {
    // the binning Position for R-type may have an offset
    return (center(gctx) + m_volumeBounds->binningOffset(bValue));
  }
  // return the center
  return center(gctx);
}

void Acts::DetectorVolume::createBoundaryPortals() {
  // Transform Surfaces To BoundarySurfaces
  auto bSurfaces = m_volumeBounds->decompose(m_transform);
  m_boundaryPortals.reserve(bSurfaces.size());
  for (auto& bsf : bSurfaces) {
    // The nominal normal vector
    auto defaultContext = GeometryContext();
    Vector3D dvReference = binningPosition(defaultContext, binR);
    Vector3D sfReference = bsf->binningPosition(defaultContext, binR);
    Vector3D sfNormal = bsf->normal(defaultContext, sfReference);
    const DetectorVolume* along = nullptr;
    const DetectorVolume* opposite = nullptr;
    if ((dvReference - sfReference).dot(sfNormal) > 0.) {
      along = this;
    } else {
      opposite = this;
    }
    m_boundaryPortals.push_back(
        std::make_shared<BoundaryPortal>(std::move(bsf), opposite, along));
  }
}

void Acts::DetectorVolume::attach(DetectorVolumePtr& dvolume,
                                  bool stitch) {
  bool attached = false;
  for (auto& myPortal : m_boundaryPortals) {
    for (auto& theirPortal : dvolume->m_boundaryPortals) {
      const auto& mySurface = myPortal->surfaceRepresentation();
      const auto& theirSurface = theirPortal->surfaceRepresentation();
      // Exchange the surface and attach the volume
      if (mySurface == theirSurface) {
        attachPortal(myPortal, theirPortal);
        attached = true;
      } else if (stitch) {
        // Stich the surfaces that can be stitched
        stitchPortal(myPortal, theirPortal);
      }
    }
  }
  if (not attached) {
    throw std::domain_error("DetectorVolume: DetectorVolumes can not attach.");
  }
}
