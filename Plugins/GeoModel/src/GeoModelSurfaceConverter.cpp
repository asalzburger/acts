
// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelSurfaceConverter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoShape.h>

Acts::GeoModelSensitiveSurface
Acts::GeoModelSurfaceConverter::convertToSensitiveSurface(
    const GeoFullPhysVol& geoPhysVol) {
  const GeoLogVol* logVol = geoPhysVol.getLogVol();

  const auto& transform = geoPhysVol.getAbsoluteTransform(nullptr);
  auto fullPhysVol = dynamic_cast<const GeoFullPhysVol*>(&geoPhysVol);


  if (logVol != nullptr) {
    const GeoShape* geoShape = logVol->getShape();
    if (geoShape != nullptr) {
      Vector3 translation = transform.translation();
      RotationMatrix3 rotation = transform.rotation();

      Transform3 surfaceTransform = Acts::Transform3::Identity();
      surfaceTransform.translation() = translation;

      // Try if its a box
      auto geoBox = dynamic_cast<const GeoBox*>(geoShape);
      if (geoBox != nullptr) {

        std::vector<ActsScalar> halfLengths = {geoBox->getXHalfLength(),
                                               geoBox->getYHalfLength(),
                                               geoBox->getZHalfLength()};
        // Create the surface
        auto minElement =
            std::min_element(halfLengths.begin(), halfLengths.end());
        auto zIndex = std::distance(halfLengths.begin(), minElement);
        std::size_t yIndex = zIndex > 0u ? zIndex - 1u : 2u;
        std::size_t xIndex = yIndex > 0u ? yIndex - 1u : 2u;

        Vector3 colX = rotation.col(xIndex);
        Vector3 colY = rotation.col(yIndex);
        Vector3 colZ = rotation.col(zIndex);
        rotation.col(0) = colX;
        rotation.col(1) = colY;
        rotation.col(2) = colZ;        
        surfaceTransform.linear() = rotation;

        // Create the surface
        ActsScalar halfX = halfLengths[xIndex];
        ActsScalar halfY = halfLengths[yIndex];
        // Create the surface
        auto rectangleBounds =
            std::make_shared<Acts::RectangleBounds>(halfX, halfY);

        auto detectorElement = std::make_shared<Acts::GeoModelDetectorElement>(
            geoPhysVol, rectangleBounds, surfaceTransform, 2 * halfLengths[zIndex]);
        auto surface = detectorElement->surface().getSharedPtr();
        return std::make_tuple(detectorElement, surface);
      }
    }
  }

  return std::make_tuple(nullptr, nullptr);
}
