// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Material/TrackingGeometryMaterial.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <algorithm>
#include <map>
#include <numbers>
#include <vector>

// Convenience shorthand

namespace Acts {

/// @brief Material decorator using a map as input
///
/// This reads in map with binning information to decorate the detector with
/// proto-material for material mapping. This allows us to change the mapping
/// parameters directly in the C++ code. Takes a tracking geometry in input, all
/// the surface with `mapMaterial=true` will be added to a binning map.
class MappingMaterialDecorator : public IMaterialDecorator {
 public:
  using BinningMap = std::map<std::uint64_t, std::pair<int, int>>;

  MappingMaterialDecorator(const Acts::TrackingGeometry& tGeometry,
                           Acts::Logging::Level level)
      : m_logger{getDefaultLogger("MappingMaterialDecorator", level)} {
    volumeLoop(tGeometry.highestTrackingVolume());
  }

  /// Decorate a surface
  ///
  /// @param surface the non-cost surface that is decorated
  void decorate(Surface& surface) const final {
    ACTS_VERBOSE("Processing surface: " << surface.geometryId());
    // Clear the material if registered to do so
    // Try to find the surface in the map
    auto bins = m_binningMap.find(surface.geometryId().value());
    if (bins != m_binningMap.end()) {
      ACTS_VERBOSE("-> Found material for surface, assigning");
      surface.assignSurfaceMaterial(
          binnedSurfaceMaterial(surface.getSharedPtr()));
    }
  }

  /// Decorate a TrackingVolume
  ///
  /// @param volume the non-cost volume that is decorated
  void decorate(TrackingVolume& volume) const final {
    ACTS_VERBOSE("Processing volume: " << volume.geometryId());
    // Try to find the volume in the map
    auto vMaterial = m_volumeMaterialMap.find(volume.geometryId());
    if (vMaterial != m_volumeMaterialMap.end()) {
      ACTS_VERBOSE("-> Found material for volume, assigning");
      volume.assignVolumeMaterial(vMaterial->second);
    }
  }

  /// Loop over all subvolumes and there surfaces and add all the surface with
  /// protomaterial to the binning map.
  ///
  /// @param volume to be looped onto
  void volumeLoop(const Acts::TrackingVolume* tVolume) {
    auto sameId = [tVolume](const auto& pair) {
      return (tVolume->geometryId() == pair.first);
    };
    if (std::ranges::any_of(m_volumeMaterialMap, sameId)) {
      // this volume was already visited
      return;
    }
    if (tVolume->volumeMaterial() != nullptr) {
      m_volumeMaterialMap.insert(
          {tVolume->geometryId(), tVolume->volumeMaterialPtr()});
    }
    // there are confined volumes
    if (tVolume->confinedVolumes() != nullptr) {
      // get through the volumes
      auto& volumes = tVolume->confinedVolumes()->arrayObjects();
      // loop over the volumes
      for (auto& vol : volumes) {
        // recursive call
        volumeLoop(vol.get());
      }
    }
    // there are dense volumes
    if (!tVolume->denseVolumes().empty()) {
      // loop over the volumes
      for (auto& vol : tVolume->denseVolumes()) {
        // recursive call
        volumeLoop(vol.get());
      }
    }
    if (tVolume->confinedLayers() != nullptr) {
      // get the layers
      auto& layers = tVolume->confinedLayers()->arrayObjects();
      // loop over the layers
      for (auto& lay : layers) {
        auto& layRep = lay->surfaceRepresentation();
        if (layRep.surfaceMaterial() != nullptr &&
            layRep.geometryId() != GeometryIdentifier()) {
          m_binningMap.insert(
              {layRep.geometryId().value(), std::make_pair(1, 1)});
        }
        if (lay->approachDescriptor() != nullptr) {
          for (auto& asf : lay->approachDescriptor()->containedSurfaces()) {
            if (asf->surfaceMaterial() != nullptr) {
              m_binningMap.insert(
                  {asf->geometryId().value(), std::make_pair(1, 1)});
            }
          }
        }
        if (lay->surfaceArray() != nullptr) {
          for (auto& ssf : lay->surfaceArray()->surfaces()) {
            if (ssf->surfaceMaterial() != nullptr) {
              m_binningMap.insert(
                  {ssf->geometryId().value(), std::make_pair(1, 1)});
            }
          }
        }
      }
    }
    // Let's finally check the boundaries
    for (auto& bsurf : tVolume->boundarySurfaces()) {
      // the surface representation
      auto& bssfRep = bsurf->surfaceRepresentation();
      if (bssfRep.geometryId().volume() == tVolume->geometryId().volume()) {
        if (bssfRep.surfaceMaterial() != nullptr) {
          m_binningMap.insert(
              {bssfRep.geometryId().value(), std::make_pair(1, 1)});
        }
      }
    }
  }

  /// Add protomaterial to a surface bases on the binning map
  ///
  /// @param surface protomaterial will be added to
  std::shared_ptr<const Acts::ISurfaceMaterial> binnedSurfaceMaterial(
      const std::shared_ptr<const Acts::Surface>& surface) const {
    auto bin = m_binningMap.find(surface->geometryId().value());
    std::vector<Acts::DirectedProtoAxis> dProtoAxes;
    if (bin == m_binningMap.end()) {
      ACTS_ERROR("No corresponding binning in the map to surface "
                 << surface->geometryId());
    } else {
      auto binning = bin->second;
      // Check which type of bounds is associated to the surface
      const Acts::SurfaceBounds& surfaceBounds = surface->bounds();
      const Acts::RadialBounds* radialBounds =
          dynamic_cast<const Acts::RadialBounds*>(&surfaceBounds);
      const Acts::CylinderBounds* cylinderBounds =
          dynamic_cast<const Acts::CylinderBounds*>(&surfaceBounds);
      const Acts::AnnulusBounds* annulusBounds =
          dynamic_cast<const Acts::AnnulusBounds*>(&surfaceBounds);
      const Acts::RectangleBounds* rectangleBounds =
          dynamic_cast<const Acts::RectangleBounds*>(&surfaceBounds);
      const Acts::TrapezoidBounds* trapezoidBounds =
          dynamic_cast<const Acts::TrapezoidBounds*>(&surfaceBounds);

      if (radialBounds != nullptr) {
        dProtoAxes.emplace_back(
            Acts::AxisDirection::AxisPhi,
            (radialBounds->get(Acts::RadialBounds::eHalfPhiSector) -
             std::numbers::pi) < Acts::s_epsilon
                ? Acts::AxisBoundaryType::Closed
                : Acts::AxisBoundaryType::Open,
            radialBounds->get(Acts::RadialBounds::eAveragePhi) -
                radialBounds->get(Acts::RadialBounds::eHalfPhiSector),
            radialBounds->get(Acts::RadialBounds::eAveragePhi) +
                radialBounds->get(Acts::RadialBounds::eHalfPhiSector),
            static_cast<std::size_t>(binning.first));
        dProtoAxes.emplace_back(Acts::AxisDirection::AxisR,
                                Acts::AxisBoundaryType::Open,
                                radialBounds->rMin(), radialBounds->rMax(),
                                static_cast<std::size_t>(binning.second));
      }
      if (cylinderBounds != nullptr) {
        dProtoAxes.emplace_back(
            Acts::AxisDirection::AxisPhi,
            (cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector) -
             std::numbers::pi) < Acts::s_epsilon
                ? Acts::AxisBoundaryType::Closed
                : Acts::AxisBoundaryType::Open,
            cylinderBounds->get(Acts::CylinderBounds::eAveragePhi) -
                cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector),
            cylinderBounds->get(Acts::CylinderBounds::eAveragePhi) +
                cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector),
            static_cast<std::size_t>(binning.first));
        dProtoAxes.emplace_back(
            Acts::AxisDirection::AxisZ, Acts::AxisBoundaryType::Open,
            -1 * cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ),
            cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ),
            static_cast<std::size_t>(binning.second));
      }
      if (annulusBounds != nullptr) {
        dProtoAxes.emplace_back(
            Acts::AxisDirection::AxisPhi, Acts::AxisBoundaryType::Open,
            annulusBounds->get(Acts::AnnulusBounds::eMinPhiRel),
            annulusBounds->get(Acts::AnnulusBounds::eMaxPhiRel),
            static_cast<std::size_t>(binning.first));
        dProtoAxes.emplace_back(Acts::AxisDirection::AxisR,
                                Acts::AxisBoundaryType::Open,
                                annulusBounds->rMin(), annulusBounds->rMax(),
                                static_cast<std::size_t>(binning.second));
      }
      if (rectangleBounds != nullptr) {
        dProtoAxes.emplace_back(
            Acts::AxisDirection::AxisX, Acts::AxisBoundaryType::Open,
            rectangleBounds->get(Acts::RectangleBounds::eMinX),
            rectangleBounds->get(Acts::RectangleBounds::eMaxX),
            static_cast<std::size_t>(binning.first));
        dProtoAxes.emplace_back(
            Acts::AxisDirection::AxisY, Acts::AxisBoundaryType::Open,
            rectangleBounds->get(Acts::RectangleBounds::eMinY),
            rectangleBounds->get(Acts::RectangleBounds::eMaxY),
            static_cast<std::size_t>(binning.second));
      }
      if (trapezoidBounds != nullptr) {
        double halfLengthX = std::max(
            trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthXnegY),
            trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthXposY));
        dProtoAxes.emplace_back(Acts::AxisDirection::AxisX,
                                Acts::AxisBoundaryType::Open,
                                static_cast<double>(-1 * halfLengthX),
                                static_cast<double>(halfLengthX),
                                static_cast<std::size_t>(binning.first));
        dProtoAxes.emplace_back(
            Acts::AxisDirection::AxisY, Acts::AxisBoundaryType::Open,
            -1 * trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthY),
            trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthY),
            static_cast<std::size_t>(binning.second));
      }
    }
    return std::make_shared<Acts::ProtoSurfaceMaterial>(dProtoAxes);
  }

  /// Readonly access to the BinningMap
  BinningMap& binningMap() { return m_binningMap; }

  /// set the binning map
  void setBinningMap(BinningMap binningMap) {
    m_binningMap = std::move(binningMap);
  }

 private:
  BinningMap m_binningMap;

  VolumeMaterialMaps m_volumeMaterialMap;

  std::unique_ptr<const Logger> m_logger;

  const Logger& logger() const { return *m_logger; }
};
}  // namespace Acts
