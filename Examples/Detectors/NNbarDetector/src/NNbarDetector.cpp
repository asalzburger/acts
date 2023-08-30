// This file is part of the Acts project.
//
// Copyright (C) 2022-2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/NNbarDetector/NNbarDetector.hpp"

#include "Acts/Detector/KdtSurfacesProvider.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/PlaneLayer.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"
#include "Acts/Plugins/Geant4/Geant4PhysicalVolumeSelectors.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/StringHelpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/Geant4/GdmlDetectorConstruction.hpp"

#include <ostream>
#include <stdexcept>

#include "G4VPhysicalVolume.hh"

namespace {

/// A surface sorting module
template <Acts::BinningValue bVALUE>
struct SurfaceSorter {
  Acts::GeometryContext sortContext;

  bool operator()(const std::shared_ptr<Acts::Surface>& s1,
                  const std::shared_ptr<Acts::Surface>& s2) const {
    return Acts::VectorHelpers::cast(s1->center(sortContext), bVALUE) <
           Acts::VectorHelpers::cast(s2->center(sortContext), bVALUE);
  }
};

auto surfacesPerLayer(
    const Acts::GeometryContext& buildContext,
    const std::vector<std::shared_ptr<Acts::Surface>>& surfaces,
    Acts::BinningValue bval, Acts::ActsScalar thickness) {
  std::vector<std::vector<std::shared_ptr<Acts::Surface>>> sPerLayer;
  Acts::ActsScalar layerReference = 0.;
  std::vector<std::shared_ptr<Acts::Surface>> layerSurfaces;
  for (const auto& s : surfaces) {
    Acts::ActsScalar surfaceReference =
        Acts::VectorHelpers::cast(s->center(buildContext), bval);
    if (layerSurfaces.empty()) {
      layerReference = surfaceReference;
    }
    if (std::abs(surfaceReference - layerReference) > thickness) {
      sPerLayer.push_back(std::move(layerSurfaces));
      layerSurfaces.clear();
      layerReference = surfaceReference;
    }
    layerSurfaces.push_back(s);
  }
  return sPerLayer;
};

auto createPlaneLayer(
    const Acts::GeometryContext& buildContext,
    const std::vector<std::shared_ptr<Acts::Surface>>& surfaces,
    const std::array<Acts::ActsScalar, 2u>& rectangleBounds,
    Acts::BinningValue bval, Acts::ActsScalar thickness) {
  Acts::ActsScalar layerPosition = 0.;
  for (const auto& s : surfaces) {
    layerPosition += Acts::VectorHelpers::cast(s->center(buildContext), bval);
  }
  // The layer position
  layerPosition /= surfaces.size();
  Acts::Vector3 layerPosition3(0., 0., 0.);
  layerPosition3[bval] = layerPosition;
  // Grab the rotation from the first surface
  auto rotation = surfaces.front()->transform(buildContext).rotation();

  Acts::Transform3 layerTransform = Acts::Transform3::Identity();
  layerTransform.rotate(rotation);
  layerTransform.pretranslate(layerPosition3);

  auto allSurfaces = std::make_unique<Acts::SurfaceArray::SingleElementLookup>(
      Acts::unpack_shared_const_vector(surfaces));

  std::vector<std::shared_ptr<const Acts::Surface>> surfacesConst;
  surfacesConst.reserve(surfaces.size());
  surfacesConst.insert(surfacesConst.end(), surfaces.begin(), surfaces.end());
  auto surfaceArray = std::make_unique<Acts::SurfaceArray>(
      std::move(allSurfaces), surfacesConst);

  auto rectangle = std::make_shared<const Acts::RectangleBounds>(
      rectangleBounds[0], rectangleBounds[1]);

  return Acts::PlaneLayer::create(layerTransform, rectangle,
                                  std::move(surfaceArray), thickness, nullptr,
                                  Acts::LayerType::active);
};

auto createPlaneLayers(
    const Acts::GeometryContext& buildContext,
    const std::vector<std::vector<std::shared_ptr<Acts::Surface>>>&
        surfacesPerLayer,
    const std::array<Acts::ActsScalar, 2u>& rectangleBounds,
    Acts::BinningValue bval, Acts::ActsScalar thickness) {
  std::vector<std::shared_ptr<const Acts::Layer>> layers;
  layers.reserve(surfacesPerLayer.size());
  for (const auto& spl : surfacesPerLayer) {
    layers.push_back(
        createPlaneLayer(buildContext, spl, rectangleBounds, bval, thickness));
  }
  return layers;
};

}  // namespace

auto ActsExamples::NNbar::NNbarDetector::constructTrackingGeometry()
    -> std::tuple<TrackingGeometryPtr, ContextDecorators, DetectorElements> {
  ACTS_LOCAL_LOGGER(Acts::getDefaultLogger("NNbarDetector", m_cfg.logLevel));

  ACTS_INFO("Building an Acts::TrackingGeometry called '"
            << m_cfg.name << "' from gdml file '" << m_cfg.gdmlFile << "'");

  // Return objects
  TrackingGeometryPtr trackingGeometry = nullptr;
  DetectorElements elements = {};
  ContextDecorators decorators = {};

  Acts::GeometryContext buildContext;

  // Load the GDML file into a Gdml Detector Factory
  ActsExamples::GdmlDetectorConstruction gdmlDetectorConstruction(
      m_cfg.gdmlFile);
  const auto* world = gdmlDetectorConstruction.Construct();

  // --- Convert surfaces of the inner sectors
  // Create the selectors
  auto innerSensitiveSelectors =
      std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
          m_cfg.innerSensitiveMatches, false);
  auto innerPassiveSelectors =
      std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
          m_cfg.innerPassiveMatches, false);

  Acts::Geant4DetectorSurfaceFactory::Cache innerCache;
  Acts::Geant4DetectorSurfaceFactory::Options innerOptions;
  innerOptions.sensitiveSurfaceSelector = innerSensitiveSelectors;
  innerOptions.passiveSurfaceSelector = innerPassiveSelectors;

  G4Transform3D nominal;
  Acts::Geant4DetectorSurfaceFactory factory;
  factory.construct(innerCache, nominal, *world, innerOptions);

  ACTS_INFO("Inner System:  " << innerCache.sensitiveSurfaces.size()
                              << " inner sensitive surfaces and "
                              << innerCache.passiveSurfaces.size()
                              << " inner passive surfaces");

  // --- Convert surfaces of the TPC sectors
  // Create the selectors
  auto tpcSensitiveSelectors =
      std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
          m_cfg.tpcSensitiveMatches, false);
  auto tpcPassiveSelectors =
      std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
          m_cfg.tpcPassiveMatches, false);

  Acts::Geant4DetectorSurfaceFactory::Cache tpcCache;
  Acts::Geant4DetectorSurfaceFactory::Options tpcOptions;
  tpcOptions.sensitiveSurfaceSelector = tpcSensitiveSelectors;
  tpcOptions.passiveSurfaceSelector = tpcPassiveSelectors;

  factory.construct(tpcCache, nominal, *world, tpcOptions);

  ACTS_INFO("TPC   System:  " << tpcCache.sensitiveSurfaces.size()
                              << " TPC sensitive surfaces and "
                              << tpcCache.passiveSurfaces.size()
                              << " TPC passive surfaces");

  // Parse the inner system and gather components
  Acts::ActsScalar innerMaxR = 0.;
  Acts::ActsScalar innerMaxHz = 0.;
  std::vector<std::shared_ptr<const Acts::Layer>> innerLayers = {};
  for (const auto& se : innerCache.sensitiveSurfaces) {
    auto surface = std::get<1>(se);
    auto boundValues = surface->bounds().values();
    Acts::ActsScalar r = boundValues[Acts::CylinderBounds::BoundValues::eR];
    Acts::ActsScalar hZ =
        boundValues[Acts::CylinderBounds::BoundValues::eHalfLengthZ];
    innerMaxR = std::max(innerMaxR, r);
    innerMaxHz = std::max(innerMaxHz, hZ);
    auto layerBounds = std::make_shared<Acts::CylinderBounds>(r, hZ);
    auto surfaceArray = std::make_unique<Acts::SurfaceArray>(surface);
    innerLayers.push_back(Acts::CylinderLayer::create(
        Acts::Transform3::Identity(), layerBounds, std::move(surfaceArray),
        m_cfg.innerLayerThickness, nullptr, Acts::LayerType::active));
    // Cache the elements
    elements.push_back(std::get<0>(se));
  }

  // Maximum R - add the envelope
  innerMaxR += m_cfg.innerVolumeEnvelope;

  // Tooling
  Acts::LayerArrayCreator::Config lacCfg;
  Acts::LayerArrayCreator lac(
      lacCfg, Acts::getDefaultLogger("LayerArrayCreator", m_cfg.logLevel));

  Acts::TrackingVolumeArrayCreator::Config tvacCfg;
  Acts::TrackingVolumeArrayCreator tvac(
      tvacCfg,
      Acts::getDefaultLogger("TrackingVolumeArrayCreator",
                             Acts::Logging::VERBOSE));  // m_cfg.logLevel));

  // Parse the TPC system and gather componetns
  Acts::Extent tpcExtent;
  std::vector<std::shared_ptr<Acts::Surface>> tpcSurfaces;
  tpcSurfaces.reserve(tpcCache.sensitiveSurfaces.size());
  for (const auto& se : tpcCache.sensitiveSurfaces) {
    auto surface = std::get<1>(se);
    tpcExtent.extend(
        surface->polyhedronRepresentation(buildContext, 1u).extent());
    tpcSurfaces.push_back(surface);
    // Cache the elements
    elements.push_back(std::get<0>(se));
  }

  auto la = lac.layerArray(buildContext, innerLayers, 0., innerMaxR,
                           Acts::arbitrary, Acts::binR);

  ACTS_INFO("Inner System: layer array with '" << la->arrayObjects().size()
                                               << "' layers");

  Acts::ActsScalar systemMaxZ = std::max(innerMaxHz, tpcExtent.max(Acts::binR));

  // Build the inner box
  auto innerBounds = std::make_shared<Acts::CuboidVolumeBounds>(
      innerMaxR, innerMaxR, systemMaxZ);

  auto innerVolume = Acts::TrackingVolume::create(
      Acts::Transform3::Identity(), innerBounds, nullptr, std::move(la),
      nullptr, {}, "InnerSystem");

  // TPC dimension
  Acts::ActsScalar tpcHalfLengthYuo =
      0.5 * (tpcExtent.max(Acts::binY) - innerMaxR + m_cfg.tpcVolumeEnvelope);
  Acts::ActsScalar tpcPosY =
      0.5 * (tpcExtent.max(Acts::binY) + m_cfg.tpcVolumeEnvelope + innerMaxR);

  Acts::ActsScalar tpcHalfLengthXlr =
      0.5 * (tpcExtent.max(Acts::binX) - innerMaxR + m_cfg.tpcVolumeEnvelope);
  Acts::ActsScalar tpcPosX =
      0.5 * (tpcExtent.max(Acts::binX) + m_cfg.tpcVolumeEnvelope + innerMaxR);

  // KDT based lookup for TPC surfaces
  using CenterGenterator = Acts::Experimental::detail::CenterReferenceGenerator;
  Acts::Experimental::KdtSurfaces<2u, 1000u, CenterGenterator> tpcKdtSurfaces(
      buildContext, tpcSurfaces, {Acts::binX, Acts::binY}, CenterGenterator{});

  ACTS_INFO("TPC system has '" << tpcSurfaces.size() << "' surfaces");

  // This code can be written in a loop - it is here for demonstration purposes
  // to make each step of the build relatively obvious
  // -> gather surfaces for lower TPC volume, sort in Y
  Acts::RangeXD<2u, Acts::ActsScalar> loRange;
  loRange[Acts::binX].shrink(-innerMaxR, innerMaxR);
  loRange[Acts::binY].shrink(-innerMaxR - 2 * tpcHalfLengthYuo, -innerMaxR);
  ACTS_VERBOSE("TPC lower volume query range is : " << loRange.toString());
  auto loSurfaces = tpcKdtSurfaces.surfaces(loRange);
  std::sort(loSurfaces.begin(), loSurfaces.end(),
            SurfaceSorter<Acts::binY>{buildContext});
  // Create the pacakge per layer
  auto loSurfacesPerLayer = surfacesPerLayer(
      buildContext, loSurfaces, Acts::binY, m_cfg.tpcLayerThickness);
  auto loLayers = createPlaneLayers(buildContext, loSurfacesPerLayer,
                                    {systemMaxZ - m_cfg.tpcLayerThickness,
                                     innerMaxR - m_cfg.tpcLayerThickness},
                                    Acts::binY, m_cfg.tpcLayerThickness);
  auto loArray =
      lac.layerArray(buildContext, loLayers, -innerMaxR - 2 * tpcHalfLengthYuo,
                     -innerMaxR, Acts::arbitrary, Acts::binY);
  size_t nLoSurfaces = loSurfaces.size();
  ACTS_INFO("TPC lower volume has '" << nLoSurfaces << "' surfaces");
  ACTS_INFO("  - those are packaged into " << loSurfacesPerLayer.size()
                                           << " layers");

  // -> gather surfaces for higher TPC volume, sort in Y
  Acts::RangeXD<2u, Acts::ActsScalar> upRange;
  upRange[Acts::binX].shrink(-innerMaxR, innerMaxR);
  upRange[Acts::binY].shrink(innerMaxR, innerMaxR + 2 * tpcHalfLengthYuo);
  ACTS_VERBOSE("TPC upper volume query range is : " << upRange.toString());
  auto upSurfaces = tpcKdtSurfaces.surfaces(upRange);
  std::sort(upSurfaces.begin(), upSurfaces.end(),
            SurfaceSorter<Acts::binY>{buildContext});
  // Create the pacakge per layer
  auto upSurfacesPerLayer = surfacesPerLayer(
      buildContext, upSurfaces, Acts::binY, m_cfg.tpcLayerThickness);
  auto upLayers = createPlaneLayers(buildContext, upSurfacesPerLayer,
                                    {systemMaxZ - m_cfg.tpcLayerThickness,
                                     innerMaxR - m_cfg.tpcLayerThickness},
                                    Acts::binY, m_cfg.tpcLayerThickness);
  auto upArray = lac.layerArray(buildContext, upLayers, innerMaxR,
                                innerMaxR + 2 * tpcHalfLengthYuo,
                                Acts::arbitrary, Acts::binY);

  size_t nUpSurfaces = upSurfaces.size();
  ACTS_INFO("TPC upper volume has '" << nUpSurfaces << "' surfaces");
  ACTS_INFO("  - those are packaged into " << upSurfacesPerLayer.size()
                                           << " layers");

  // -> gather surfaces for left sided TPC volume, sort in X
  Acts::RangeXD<2u, Acts::ActsScalar> leRange;
  leRange[Acts::binX].shrink(-innerMaxR - 2 * tpcHalfLengthXlr, -innerMaxR);
  leRange[Acts::binY].shrink(-innerMaxR - 2 * tpcHalfLengthYuo,
                             innerMaxR + 2 * tpcHalfLengthYuo);
  ACTS_VERBOSE("TPC left  volume query range is : " << leRange.toString());
  auto leSurfaces = tpcKdtSurfaces.surfaces(leRange);
  std::sort(leSurfaces.begin(), leSurfaces.end(),
            SurfaceSorter<Acts::binX>{buildContext});
  // Create the pacakge per layer
  auto leSurfacesPerLayer = surfacesPerLayer(
      buildContext, leSurfaces, Acts::binX, m_cfg.tpcLayerThickness);
  auto leLayers = createPlaneLayers(
      buildContext, leSurfacesPerLayer,
      {innerMaxR + 2 * tpcHalfLengthYuo - m_cfg.tpcLayerThickness,
       systemMaxZ - m_cfg.tpcLayerThickness},
      Acts::binX, m_cfg.tpcLayerThickness);
  auto leArray =
      lac.layerArray(buildContext, leLayers, -innerMaxR - 2 * tpcHalfLengthXlr,
                     -innerMaxR, Acts::arbitrary, Acts::binX);
  size_t nLeSurfaces = leSurfaces.size();
  ACTS_INFO("TPC left  volume has '" << nLeSurfaces << "' surfaces");
  ACTS_INFO("  - those are packaged into " << leSurfacesPerLayer.size()
                                           << " layers");

  // -> gather surfaces for right sided TPC volume, sort in X
  Acts::RangeXD<2u, Acts::ActsScalar> riRange;
  riRange[Acts::binX].shrink(innerMaxR, innerMaxR + 2 * tpcHalfLengthXlr);
  riRange[Acts::binY].shrink(-innerMaxR - 2 * tpcHalfLengthYuo,
                             innerMaxR + 2 * tpcHalfLengthYuo);
  ACTS_VERBOSE("TPC right volume query range is : " << riRange.toString());
  auto riSurfaces = tpcKdtSurfaces.surfaces(riRange);
  std::sort(riSurfaces.begin(), riSurfaces.end(),
            SurfaceSorter<Acts::binX>{buildContext});
  // Create the pacakge per layer
  auto riSurfacesPerLayer = surfacesPerLayer(
      buildContext, riSurfaces, Acts::binX, m_cfg.tpcLayerThickness);
  auto riLayers = createPlaneLayers(
      buildContext, riSurfacesPerLayer,
      {innerMaxR + 2 * tpcHalfLengthYuo - m_cfg.tpcLayerThickness,
       systemMaxZ - m_cfg.tpcLayerThickness},
      Acts::binX, m_cfg.tpcLayerThickness);
  auto riArray = lac.layerArray(buildContext, riLayers, innerMaxR,
                                innerMaxR + 2 * tpcHalfLengthXlr,
                                Acts::arbitrary, Acts::binX);
  size_t nRiSurfaces = riSurfaces.size();
  ACTS_INFO("TPC right volume has '" << nRiSurfaces << "' surfaces");
  ACTS_INFO("  - those are packaged into " << riSurfacesPerLayer.size()
                                           << " layers");

  // Bail out if something went wrong
  if (nLoSurfaces + nUpSurfaces + nLeSurfaces + nRiSurfaces !=
      tpcSurfaces.size()) {
    throw std::runtime_error("TPC surface count mismatch.");
  }

  // Build the upper and lower TPC box
  auto tpcULBounds = std::make_shared<Acts::CuboidVolumeBounds>(
      innerMaxR, tpcHalfLengthYuo, systemMaxZ);

  // Uper lower TPC volume position
  auto tpcLoPosition = Acts::Transform3::Identity();
  tpcLoPosition.translate(Acts::Vector3(0., -tpcPosY, 0.));

  auto tpcLoVolume =
      Acts::TrackingVolume::create(tpcLoPosition, tpcULBounds, nullptr,
                                   std::move(loArray), nullptr, {}, "TPCLower");

  // Upper volume position
  auto tpcUpPosition = Acts::Transform3::Identity();
  tpcUpPosition.translate(Acts::Vector3(0., tpcPosY, 0.));
  auto tpcUpVolume =
      Acts::TrackingVolume::create(tpcUpPosition, tpcULBounds, nullptr,
                                   std::move(upArray), nullptr, {}, "TPCUpper");

  // Lower volume

  // Glue them togeter in Y
  tpcLoVolume->glueTrackingVolume(
      buildContext, Acts::BoundarySurfaceFace::positiveFaceZX,
      innerVolume.get(), Acts::BoundarySurfaceFace::negativeFaceZX);
  innerVolume->glueTrackingVolume(
      buildContext, Acts::BoundarySurfaceFace::positiveFaceZX,
      tpcUpVolume.get(), Acts::BoundarySurfaceFace::negativeFaceZX);

  auto lIu = tvac.trackingVolumeArray(
      buildContext, {tpcLoVolume, innerVolume, tpcUpVolume}, Acts::binY);

  // Package the low and upper TPC volume with the inner system into one
  // container
  auto lIuVolumeBounds = std::make_shared<Acts::CuboidVolumeBounds>(
      innerMaxR, innerMaxR + 2 * tpcHalfLengthYuo, systemMaxZ);
  auto lIuVolume = Acts::TrackingVolume::create(Acts::Transform3::Identity(),
                                                lIuVolumeBounds, std::move(lIu),
                                                "InnerSystemAndTPCUL");

  // Left Right volume
  auto tpcLRBounds = std::make_shared<Acts::CuboidVolumeBounds>(
      tpcHalfLengthXlr, innerMaxR + 2 * tpcHalfLengthYuo, systemMaxZ);

  // Left volume
  auto tpcLePosition = Acts::Transform3::Identity();
  tpcLePosition.pretranslate(Acts::Vector3(-tpcPosX, 0., 0.));
  auto tpcLeVolume =
      Acts::TrackingVolume::create(tpcLePosition, tpcLRBounds, nullptr,
                                   std::move(leArray), nullptr, {}, "TPCLeft");

  // Right volume
  auto tpcRiPosition = Acts::Transform3::Identity();
  tpcRiPosition.pretranslate(Acts::Vector3(tpcPosX, 0., 0.));
  auto tpcRiVolume =
      Acts::TrackingVolume::create(tpcRiPosition, tpcLRBounds, nullptr,
                                   std::move(riArray), nullptr, {}, "TPCRight");

  // Glue them togeter in X - a bit more complicated
  tpcLeVolume->glueTrackingVolumes(
      buildContext, Acts::BoundarySurfaceFace::positiveFaceYZ,
      std::const_pointer_cast<Acts::TrackingVolumeArray>(lIu),
      Acts::BoundarySurfaceFace::negativeFaceYZ);

  tpcRiVolume->glueTrackingVolumes(
      buildContext, Acts::BoundarySurfaceFace::negativeFaceYZ,
      std::const_pointer_cast<Acts::TrackingVolumeArray>(lIu),
      Acts::BoundarySurfaceFace::positiveFaceYZ);

  // The full tracking package
  auto llIurVolumeBounds = std::make_shared<Acts::CuboidVolumeBounds>(
      innerMaxR + 2 * tpcHalfLengthXlr, innerMaxR + 2 * tpcHalfLengthYuo,
      systemMaxZ);
  auto llIur = tvac.trackingVolumeArray(
      buildContext, {tpcLeVolume, lIuVolume, tpcRiVolume}, Acts::binX);

  auto llIurVolume = Acts::TrackingVolume::create(
      Acts::Transform3::Identity(), llIurVolumeBounds, std::move(llIur),
      "InnerSystemAndTPC");

  trackingGeometry =
      std::make_shared<const Acts::TrackingGeometry>(llIurVolume);

  return std::tie(trackingGeometry, decorators, elements);
}
