// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"

namespace Acts {

class DetectorVolume;

class ContainerStructure {
 public:
  // Defaulted destructor
  ~ContainerStructure() = default;

  /// Return the bounds as a plain pointer
  /// @note these bounds will be assigned to the DetectorVolume
  const VolumeBounds* volumeBounds() const;

  /// Return the detector volume (next one stepped down) of the container
  const DetectorVolume* detectorVolume(const GeometryContext& gctx,
                                       const Vector3D& position) const;

 private:
  /// The volume bounds describing this layer
  std::unique_ptr<VolumeBounds> m_volumeBounds;
};

inline const VolumeBounds* ContainerStructure::volumeBounds() const {
  return m_volumeBounds.get();
}

inline const DetectorVolume* ContainerStructure::detectorVolume(
    const GeometryContext& /*gctx*/, const Vector3D& /*position*/) const {
  return nullptr;
}

}  // namespace Acts