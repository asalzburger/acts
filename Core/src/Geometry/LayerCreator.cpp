// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/LayerCreator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/PlaneLayer.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <set>
#include <utility>

namespace Acts {

using VectorHelpers::perp;
using VectorHelpers::phi;

LayerCreator::LayerCreator(const LayerCreator::Config& lcConfig,
                           std::unique_ptr<const Logger> logger)
    : m_cfg(lcConfig), m_logger(std::move(logger)) {}

void LayerCreator::setConfiguration(const LayerCreator::Config& lcConfig) {
  // @todo check consistency
  // copy the configuration
  m_cfg = lcConfig;
}

void LayerCreator::setLogger(std::unique_ptr<const Logger> newLogger) {
  m_logger = std::move(newLogger);
}

MutableLayerPtr LayerCreator::cylinderLayer(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t binsPhi,
    std::size_t binsZ, std::optional<ProtoLayer> _protoLayer,
    const Transform3& transform, std::unique_ptr<ApproachDescriptor> ad) const {
  ProtoLayer protoLayer =
      _protoLayer ? *_protoLayer : ProtoLayer(gctx, surfaces);
  if (!_protoLayer) {
    protoLayer.envelope[AxisDirection::AxisR] = m_cfg.defaultEnvelopeR;
    protoLayer.envelope[AxisDirection::AxisZ] = m_cfg.defaultEnvelopeZ;
  }

  // Remaining layer parameters - they include the envelopes
  double layerR = protoLayer.medium(AxisDirection::AxisR);
  double layerZ = protoLayer.medium(AxisDirection::AxisZ);
  double layerHalfZ = 0.5 * protoLayer.range(AxisDirection::AxisZ);
  double layerThickness = protoLayer.range(AxisDirection::AxisR);

  ACTS_VERBOSE("Creating a cylindrical Layer:");
  ACTS_VERBOSE(" - with layer R     = " << layerR);
  ACTS_VERBOSE(" - from R min/max   = "
               << protoLayer.min(AxisDirection::AxisR, false) << " / "
               << protoLayer.max(AxisDirection::AxisR, false));
  ACTS_VERBOSE(" - with R thickness = " << layerThickness);
  ACTS_VERBOSE("   - incl envelope  = "
               << protoLayer.envelope[AxisDirection::AxisR][0u] << " / "
               << protoLayer.envelope[AxisDirection::AxisR][1u]);

  ACTS_VERBOSE(" - with z min/max   = "
               << protoLayer.min(AxisDirection::AxisZ, false) << " (-"
               << protoLayer.envelope[AxisDirection::AxisZ][0u] << ") / "
               << protoLayer.max(AxisDirection::AxisZ, false) << " (+"
               << protoLayer.envelope[AxisDirection::AxisZ][1u] << ")");

  ACTS_VERBOSE(" - z center         = " << layerZ);
  ACTS_VERBOSE(" - halflength z     = " << layerHalfZ);

  // create the layer transforms if not given
  // we need to transform in case layerZ != 0, so that the layer will be
  // correctly defined using the halflength
  Translation3 addTranslation(0., 0., 0.);
  if (transform.isApprox(Transform3::Identity())) {
    addTranslation = Translation3(0., 0., layerZ);
    ACTS_VERBOSE(" - layer z shift  = " << -layerZ);
  }

  ACTS_VERBOSE(" - with phi min/max = "
               << protoLayer.min(AxisDirection::AxisPhi, false) << " / "
               << protoLayer.max(AxisDirection::AxisPhi, false));
  ACTS_VERBOSE(" - # of modules     = " << surfaces.size() << " ordered in ( "
                                        << binsPhi << " x " << binsZ << ")");
  std::unique_ptr<SurfaceArray> sArray;
  if (!surfaces.empty()) {
    sArray = m_cfg.surfaceArrayCreator->surfaceArrayOnCylinder(
        gctx, std::move(surfaces), binsPhi, binsZ, protoLayer);

    checkBinning(gctx, *sArray);
  }

  // create the layer and push it back
  std::shared_ptr<const CylinderBounds> cBounds(
      new CylinderBounds(layerR, layerHalfZ));

  // create the layer
  MutableLayerPtr cLayer = CylinderLayer::create(
      addTranslation * transform, cBounds, std::move(sArray), layerThickness,
      std::move(ad), active);

  if (!cLayer) {
    ACTS_ERROR("Creation of cylinder layer did not succeed!");
  }
  associateSurfacesToLayer(*cLayer);

  // now return
  return cLayer;
}

MutableLayerPtr LayerCreator::cylinderLayer(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<const Surface>> surfaces, BinningType bTypePhi,
    BinningType bTypeZ, std::optional<ProtoLayer> _protoLayer,
    const Transform3& transform, std::unique_ptr<ApproachDescriptor> ad) const {
  ProtoLayer protoLayer =
      _protoLayer ? *_protoLayer : ProtoLayer(gctx, surfaces);
  if (!_protoLayer) {
    protoLayer.envelope[AxisDirection::AxisR] = m_cfg.defaultEnvelopeR;
    protoLayer.envelope[AxisDirection::AxisZ] = m_cfg.defaultEnvelopeZ;
  }

  // remaining layer parameters
  double layerR = protoLayer.medium(AxisDirection::AxisR);
  double layerZ = protoLayer.medium(AxisDirection::AxisZ);
  double layerHalfZ = 0.5 * protoLayer.range(AxisDirection::AxisZ);
  double layerThickness = protoLayer.range(AxisDirection::AxisR);

  // adjust the layer radius
  ACTS_VERBOSE("Creating a cylindrical Layer:");
  ACTS_VERBOSE(" - with layer R     = " << layerR);
  ACTS_VERBOSE(" - from R min/max   = "
               << protoLayer.min(AxisDirection::AxisR, false) << " / "
               << protoLayer.max(AxisDirection::AxisR, false));
  ACTS_VERBOSE(" - with R thickness = " << layerThickness);
  ACTS_VERBOSE("   - incl envelope  = "
               << protoLayer.envelope[AxisDirection::AxisR][0u] << " / "
               << protoLayer.envelope[AxisDirection::AxisR][1u]);
  ACTS_VERBOSE(" - with z min/max   = "
               << protoLayer.min(AxisDirection::AxisZ, false) << " (-"
               << protoLayer.envelope[AxisDirection::AxisZ][0u] << ") / "
               << protoLayer.max(AxisDirection::AxisZ, false) << " (+"
               << protoLayer.envelope[AxisDirection::AxisZ][1u] << ")");
  ACTS_VERBOSE(" - z center         = " << layerZ);
  ACTS_VERBOSE(" - halflength z     = " << layerHalfZ);

  // create the layer transforms if not given
  // we need to transform in case layerZ != 0, so that the layer will be
  // correctly defined using the halflength
  // create the layer transforms if not given
  Translation3 addTranslation(0., 0., 0.);
  if (transform.isApprox(Transform3::Identity()) && bTypeZ == equidistant) {
    addTranslation = Translation3(0., 0., layerZ);
    ACTS_VERBOSE(" - layer z shift    = " << -layerZ);
  }

  ACTS_VERBOSE(" - with phi min/max = "
               << protoLayer.min(AxisDirection::AxisPhi, false) << " / "
               << protoLayer.max(AxisDirection::AxisPhi, false));
  ACTS_VERBOSE(" - # of modules     = " << surfaces.size() << "");

  // create the surface array
  std::unique_ptr<SurfaceArray> sArray;
  if (!surfaces.empty()) {
    sArray = m_cfg.surfaceArrayCreator->surfaceArrayOnCylinder(
        gctx, std::move(surfaces), bTypePhi, bTypeZ, protoLayer);

    checkBinning(gctx, *sArray);
  }

  // create the layer and push it back
  std::shared_ptr<const CylinderBounds> cBounds(
      new CylinderBounds(layerR, layerHalfZ));

  // create the layer
  MutableLayerPtr cLayer = CylinderLayer::create(
      addTranslation * transform, cBounds, std::move(sArray), layerThickness,
      std::move(ad), active);

  if (!cLayer) {
    ACTS_ERROR("Creation of cylinder layer did not succeed!");
  }
  associateSurfacesToLayer(*cLayer);

  // now return
  return cLayer;
}

MutableLayerPtr LayerCreator::discLayer(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t binsR,
    std::size_t binsPhi, std::optional<ProtoLayer> _protoLayer,
    const Transform3& transform, std::unique_ptr<ApproachDescriptor> ad) const {
  ProtoLayer protoLayer =
      _protoLayer ? *_protoLayer : ProtoLayer(gctx, surfaces);
  if (!_protoLayer) {
    protoLayer.envelope[AxisDirection::AxisR] = m_cfg.defaultEnvelopeR;
    protoLayer.envelope[AxisDirection::AxisZ] = m_cfg.defaultEnvelopeZ;
  }

  double layerZ = protoLayer.medium(AxisDirection::AxisZ);
  double layerThickness = protoLayer.range(AxisDirection::AxisZ);

  // adjust the layer radius
  ACTS_VERBOSE("Creating a disk Layer:");
  ACTS_VERBOSE(" - at Z position    = " << layerZ);
  ACTS_VERBOSE(" - from Z min/max   = "
               << protoLayer.min(AxisDirection::AxisZ, false) << " / "
               << protoLayer.max(AxisDirection::AxisZ, false));
  ACTS_VERBOSE(" - with Z thickness = " << layerThickness);
  ACTS_VERBOSE("   - incl envelope  = "
               << protoLayer.envelope[AxisDirection::AxisZ][0u] << " / "
               << protoLayer.envelope[AxisDirection::AxisZ][1u]);
  ACTS_VERBOSE(" - with R min/max   = "
               << protoLayer.min(AxisDirection::AxisR, false) << " (-"
               << protoLayer.envelope[AxisDirection::AxisR][0u] << ") / "
               << protoLayer.max(AxisDirection::AxisR, false) << " (+"
               << protoLayer.envelope[AxisDirection::AxisR][1u] << ")");
  ACTS_VERBOSE(" - with phi min/max = "
               << protoLayer.min(AxisDirection::AxisPhi, false) << " / "
               << protoLayer.max(AxisDirection::AxisPhi, false));
  ACTS_VERBOSE(" - # of modules    = " << surfaces.size() << " ordered in ( "
                                       << binsR << " x " << binsPhi << ")");

  // create the layer transforms if not given
  Translation3 addTranslation(0., 0., 0.);
  if (transform.isApprox(Transform3::Identity())) {
    addTranslation = Translation3(0., 0., layerZ);
  }
  // create the surface array
  std::unique_ptr<SurfaceArray> sArray;
  if (!surfaces.empty()) {
    sArray = m_cfg.surfaceArrayCreator->surfaceArrayOnDisc(
        gctx, std::move(surfaces), binsR, binsPhi, protoLayer, transform);

    checkBinning(gctx, *sArray);
  }

  // create the share disc bounds
  auto dBounds = std::make_shared<const RadialBounds>(
      protoLayer.min(AxisDirection::AxisR),
      protoLayer.max(AxisDirection::AxisR));

  // create the layers
  // we use the same transform here as for the layer itself
  // for disk this is fine since we don't bin in Z, so does not matter
  MutableLayerPtr dLayer =
      DiscLayer::create(addTranslation * transform, dBounds, std::move(sArray),
                        layerThickness, std::move(ad), active);

  if (!dLayer) {
    ACTS_ERROR("Creation of disc layer did not succeed!");
  }
  associateSurfacesToLayer(*dLayer);
  // return the layer
  return dLayer;
}

MutableLayerPtr LayerCreator::discLayer(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<const Surface>> surfaces, BinningType bTypeR,
    BinningType bTypePhi, std::optional<ProtoLayer> _protoLayer,
    const Transform3& transform, std::unique_ptr<ApproachDescriptor> ad) const {
  ProtoLayer protoLayer =
      _protoLayer ? *_protoLayer : ProtoLayer(gctx, surfaces);
  if (!_protoLayer) {
    protoLayer.envelope[AxisDirection::AxisR] = m_cfg.defaultEnvelopeR;
    protoLayer.envelope[AxisDirection::AxisZ] = m_cfg.defaultEnvelopeZ;
  }

  double layerZ = protoLayer.medium(AxisDirection::AxisZ);
  double layerThickness = protoLayer.range(AxisDirection::AxisZ);

  // adjust the layer radius
  ACTS_VERBOSE("Creating a disk Layer:");
  ACTS_VERBOSE(" - at Z position    = " << layerZ);
  ACTS_VERBOSE(" - from Z min/max   = "
               << protoLayer.min(AxisDirection::AxisZ, false) << " / "
               << protoLayer.max(AxisDirection::AxisZ, false));
  ACTS_VERBOSE(" - with Z thickness = " << layerThickness);
  ACTS_VERBOSE("   - incl envelope  = "
               << protoLayer.envelope[AxisDirection::AxisZ][0u] << " / "
               << protoLayer.envelope[AxisDirection::AxisZ][1u]);
  ACTS_VERBOSE(" - with R min/max   = "
               << protoLayer.min(AxisDirection::AxisR, false) << " (-"
               << protoLayer.envelope[AxisDirection::AxisR][0u] << ") / "
               << protoLayer.max(AxisDirection::AxisR, false) << " (+"
               << protoLayer.envelope[AxisDirection::AxisR][1u] << ")");
  ACTS_VERBOSE(" - with phi min/max = "
               << protoLayer.min(AxisDirection::AxisPhi, false) << " / "
               << protoLayer.max(AxisDirection::AxisPhi, false));
  ACTS_VERBOSE(" - # of modules     = " << surfaces.size());

  // create the layer transforms if not given
  Translation3 addTranslation(0., 0., 0.);
  if (transform.isApprox(Transform3::Identity())) {
    addTranslation = Translation3(0., 0., layerZ);
  }

  // create the surface array
  std::unique_ptr<SurfaceArray> sArray;
  if (!surfaces.empty()) {
    sArray = m_cfg.surfaceArrayCreator->surfaceArrayOnDisc(
        gctx, std::move(surfaces), bTypeR, bTypePhi, protoLayer, transform);

    checkBinning(gctx, *sArray);
  }

  // create the shared disc bounds
  auto dBounds = std::make_shared<const RadialBounds>(
      protoLayer.min(AxisDirection::AxisR),
      protoLayer.max(AxisDirection::AxisR));

  // create the layers
  MutableLayerPtr dLayer =
      DiscLayer::create(addTranslation * transform, dBounds, std::move(sArray),
                        layerThickness, std::move(ad), active);
  if (!dLayer) {
    ACTS_ERROR("Creation of disc layer did not succeed!");
  }
  associateSurfacesToLayer(*dLayer);
  // return the layer
  return dLayer;
}

MutableLayerPtr LayerCreator::planeLayer(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t bins1,
    std::size_t bins2, AxisDirection aDir,
    std::optional<ProtoLayer> _protoLayer, const Transform3& transform,
    std::unique_ptr<ApproachDescriptor> ad) const {
  ProtoLayer protoLayer =
      _protoLayer ? *_protoLayer : ProtoLayer(gctx, surfaces);
  if (!_protoLayer) {
    protoLayer.envelope[AxisDirection::AxisR] = m_cfg.defaultEnvelopeR;
    protoLayer.envelope[AxisDirection::AxisZ] = m_cfg.defaultEnvelopeZ;
  }

  // remaining layer parameters
  double layerHalf1 = 0, layerHalf2 = 0, layerThickness = 0;
  switch (aDir) {
    case AxisDirection::AxisX: {
      layerHalf1 = 0.5 * (protoLayer.max(AxisDirection::AxisY) -
                          protoLayer.min(AxisDirection::AxisY));
      layerHalf2 = 0.5 * (protoLayer.max(AxisDirection::AxisZ) -
                          protoLayer.min(AxisDirection::AxisZ));
      layerThickness = (protoLayer.max(AxisDirection::AxisX) -
                        protoLayer.min(AxisDirection::AxisX));
      break;
    }
    case AxisDirection::AxisY: {
      layerHalf1 = 0.5 * (protoLayer.max(AxisDirection::AxisX) -
                          protoLayer.min(AxisDirection::AxisX));
      layerHalf2 = 0.5 * (protoLayer.max(AxisDirection::AxisZ) -
                          protoLayer.min(AxisDirection::AxisZ));
      layerThickness = (protoLayer.max(AxisDirection::AxisY) -
                        protoLayer.min(AxisDirection::AxisY));
      break;
    }
    case AxisDirection::AxisZ: {
      layerHalf1 = 0.5 * (protoLayer.max(AxisDirection::AxisX) -
                          protoLayer.min(AxisDirection::AxisX));
      layerHalf2 = 0.5 * (protoLayer.max(AxisDirection::AxisY) -
                          protoLayer.min(AxisDirection::AxisY));
      layerThickness = (protoLayer.max(AxisDirection::AxisZ) -
                        protoLayer.min(AxisDirection::AxisZ));
      break;
    }
    default:
      throw std::invalid_argument("Invalid binning value");
  }

  double centerX = 0.5 * (protoLayer.max(AxisDirection::AxisX) +
                          protoLayer.min(AxisDirection::AxisX));
  double centerY = 0.5 * (protoLayer.max(AxisDirection::AxisY) +
                          protoLayer.min(AxisDirection::AxisY));
  double centerZ = 0.5 * (protoLayer.max(AxisDirection::AxisZ) +
                          protoLayer.min(AxisDirection::AxisZ));

  ACTS_VERBOSE("Creating a plane Layer:");
  ACTS_VERBOSE(" - with layer center     = "
               << "(" << centerX << ", " << centerY << ", " << centerZ << ")");
  ACTS_VERBOSE(" - from X min/max   = "
               << protoLayer.min(AxisDirection::AxisX) << " / "
               << protoLayer.max(AxisDirection::AxisX));
  ACTS_VERBOSE(" - from Y min/max   = "
               << protoLayer.min(AxisDirection::AxisY) << " / "
               << protoLayer.max(AxisDirection::AxisY));
  ACTS_VERBOSE(" - with Z thickness = " << layerThickness);
  ACTS_VERBOSE("   - incl envelope  = " << protoLayer.envelope[aDir][0u]
                                        << " / "
                                        << protoLayer.envelope[aDir][1u]);

  // create the layer transforms if not given
  // we need to transform in case centerX/centerY/centerZ != 0, so that the
  // layer will be correctly defined
  Translation3 addTranslation(0., 0., 0.);
  if (transform.isApprox(Transform3::Identity())) {
    addTranslation = Translation3(centerX, centerY, centerZ);
    ACTS_VERBOSE(" - layer shift  = " << "(" << centerX << ", " << centerY
                                      << ", " << centerZ << ")");
  }

  std::unique_ptr<SurfaceArray> sArray;
  if (!surfaces.empty()) {
    sArray = m_cfg.surfaceArrayCreator->surfaceArrayOnPlane(
        gctx, std::move(surfaces), bins1, bins2, aDir, protoLayer, transform);

    checkBinning(gctx, *sArray);
  }

  // create the layer and push it back
  auto pBounds = std::make_shared<RectangleBounds>(layerHalf1, layerHalf2);

  // create the layer
  MutableLayerPtr pLayer =
      PlaneLayer::create(addTranslation * transform, pBounds, std::move(sArray),
                         layerThickness, std::move(ad), active);

  if (!pLayer) {
    ACTS_ERROR("Creation of plane layer did not succeed!");
  }
  associateSurfacesToLayer(*pLayer);

  // now return
  return pLayer;
}

void LayerCreator::associateSurfacesToLayer(Layer& layer) const {
  if (layer.surfaceArray() != nullptr) {
    auto surfaces = layer.surfaceArray()->surfaces();

    for (auto& surface : surfaces) {
      auto mutableSurface = const_cast<Surface*>(surface);
      mutableSurface->associateLayer(layer);
    }
  }
}

bool LayerCreator::checkBinning(const GeometryContext& gctx,
                                const SurfaceArray& sArray) const {
  // do consistency check: can we access all sensitive surfaces
  // through the binning? If not, surfaces get lost and the binning does not
  // work

  ACTS_VERBOSE("Performing consistency check");

  std::vector<const Surface*> surfaces = sArray.surfaces();
  std::set<const Surface*> sensitiveSurfaces(surfaces.begin(), surfaces.end());
  std::set<const Surface*> accessibleSurfaces;
  std::size_t nEmptyBins = 0;
  std::size_t nBinsChecked = 0;

  // iterate over all bins
  std::size_t size = sArray.size();
  for (std::size_t b = 0; b < size; ++b) {
    std::vector<const Surface*> binContent = sArray.at(b);
    // we don't check under/overflow bins
    if (!sArray.isValidBin(b)) {
      continue;
    }
    for (const auto& srf : binContent) {
      accessibleSurfaces.insert(srf);
    }
    if (binContent.empty()) {
      nEmptyBins++;
    }
    nBinsChecked++;
  }

  std::vector<const Surface*> diff;
  std::set_difference(sensitiveSurfaces.begin(), sensitiveSurfaces.end(),
                      accessibleSurfaces.begin(), accessibleSurfaces.end(),
                      std::inserter(diff, diff.begin()));

  ACTS_VERBOSE(" - Checked " << nBinsChecked << " valid bins");

  if (nEmptyBins > 0) {
    ACTS_ERROR(" -- Not all bins point to surface. " << nEmptyBins << " empty");
  } else {
    ACTS_VERBOSE(" -- All bins point to a surface");
  }

  if (!diff.empty()) {
    ACTS_ERROR(
        " -- Not all sensitive surfaces are accessible through binning. "
        "sensitive: "
        << sensitiveSurfaces.size()
        << "    accessible: " << accessibleSurfaces.size());

    // print all inaccessibles
    ACTS_ERROR(" -- Inaccessible surfaces: ");
    for (const auto& srf : diff) {
      // have to choose AxisDirection here
      Vector3 ctr = srf->referencePosition(gctx, AxisDirection::AxisR);
      ACTS_ERROR(" Surface(x=" << ctr.x() << ", y=" << ctr.y()
                               << ", z=" << ctr.z() << ", r=" << perp(ctr)
                               << ", phi=" << phi(ctr) << ")");
    }

  } else {
    ACTS_VERBOSE(" -- All sensitive surfaces are accessible through binning.");
  }

  return nEmptyBins == 0 && diff.empty();
}

}  // namespace Acts
