// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/DetectorElementBase.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>

class GeoVPhysVol;

namespace Acts {

class ISurfaceMaterial;
class Surface;

/// @class GeoModelDetectorElement
///
/// Detector element representative for GeoModel based 
/// sensitive elements.
class GeoModelDetectorElement : public DetectorElementBase {
 public:
  /// Broadcast the context type
  using ContextType = GeometryContext;

  /// @brief  Constructor with arguments
  /// @param surface the surface representing this detector element
  /// @param geoPhysVol the physical volume representing this detector element
  /// @param toGlobal the global transformation before the volume
  /// @param thickness the thickness of this detector element
  GeoModelDetectorElement(std::shared_ptr<Surface> surface,
                        const GeoVPhysVol& geoPhysVol,
                        const Transform3& toGlobal, ActsScalar thickness);

  /// Return local to global transform associated with this detector element
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  const Transform3& transform(const GeometryContext& gctx) const override;

  /// Return surface associated with this detector element
  const Surface& surface() const override;

  /// Non-const access to surface associated with this detector element
  Surface& surface() override;

  /// Return the thickness of this detector element
  ActsScalar thickness() const override;

  /// @return to the Geant4 physical volume
  const GeoVPhysVol& geoVPhysicalVolume() const;

 private:
  /// Corresponding Surface
  std::shared_ptr<Surface> m_surface;
  /// The GEant4 physical volume
  const GeoVPhysVol* m_geoPhysVol{nullptr};
  /// The global transformation before the volume
  Transform3 m_toGlobal;
  ///  Thickness of this detector element
  ActsScalar m_thickness{0.};
};

}  // namespace Acts

