// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/LayerStructure.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"

Acts::LayerStructure::LayerStructure(std::unique_ptr<VolumeBounds> volumeBounds,
                                     std::shared_ptr<Surface> surface)
    : m_volumeBounds(std::move(volumeBounds))
    , m_surfaceArrays(BinnedArrayXD<SurfaceArray>(SurfaceArray(m_containedSurfaces[0]->getSharedPtr())))
    , m_surfaces({surface}) {
  m_containedSurfaces = unpack_shared_vector_to_const(m_surfaces);
  checkConsistency();
}

Acts::LayerStructure::LayerStructure(
    std::unique_ptr<VolumeBounds> volumeBounds,
    SurfaceArray&& surfaceArray, std::vector<std::shared_ptr<Surface>> surfaces)
    : m_volumeBounds(std::move(volumeBounds))
    , m_surfaceArrays(BinnedArrayXD<SurfaceArray>(surfaceArray)) 
    , m_surfaces(surfaces) {
  m_containedSurfaces = unpack_shared_vector_to_const(m_surfaces);  
  checkConsistency();
}

Acts::LayerStructure::LayerStructure(
    std::unique_ptr<VolumeBounds> volumeBounds,
    BinnedArrayXD<SurfaceArray> surfaceArrays,
    std::vector<std::shared_ptr<Surface>> surfaces)
    : m_volumeBounds(std::move(volumeBounds)),
      m_surfaceArrays(std::move(surfaceArrays)),
      m_surfaces(surfaces) {
  m_containedSurfaces = unpack_shared_vector_const(m_surfaces);
  checkConsistency();
}
