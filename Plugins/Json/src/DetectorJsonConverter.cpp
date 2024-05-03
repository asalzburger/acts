// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/DetectorJsonConverter.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Plugins/Json/DetectorVolumeFinderJsonConverter.hpp"
#include "Acts/Plugins/Json/DetectorVolumeJsonConverter.hpp"
#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Plugins/Json/IndexedSurfacesJsonConverter.hpp"
#include "Acts/Plugins/Json/PortalJsonConverter.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"
// Binned surface material
#include "Acts/Material/BinnedSurfaceMaterial.hpp"

#include <algorithm>
#include <ctime>
#include <set>

nlohmann::json Acts::DetectorJsonConverter::toJson(
    const GeometryContext& gctx, const Experimental::Detector& detector,
    const Options& options) {
  // Get the time stamp
  std::time_t tt = 0;
  std::time(&tt);
  auto ti = std::localtime(&tt);

  nlohmann::json jDetector;

  std::size_t nSurfaces = 0;
  std::vector<const Experimental::Portal*> portals;

  for (const auto* volume : detector.volumes()) {
    nSurfaces += volume->surfaces().size();
    for (const auto& portal : volume->portals()) {
      if (std::find(portals.begin(), portals.end(), portal) == portals.end()) {
        portals.push_back(portal);
      }
    }
  }

  // Write the actual data objects
  nlohmann::json jData;
  jData["name"] = detector.name();

  // The portals are written first
  auto volumes = detector.volumes();
  nlohmann::json jPortals;

  for (const auto& portal : portals) {
    auto jPortal = PortalJsonConverter::toJson(
        gctx, *portal, volumes, options.volumeOptions.portalOptions);
    jPortals.push_back(jPortal);
  }
  jData["portals"] = jPortals;

  // The volumes are written next, with portals already defined, the volumes
  // will only index those
  nlohmann::json jVolumes;
  for (const auto& volume : volumes) {
    auto jVolume = DetectorVolumeJsonConverter::toJson(
        gctx, *volume, volumes, portals, options.volumeOptions);
    jVolumes.push_back(jVolume);
  }
  jData["volumes"] = jVolumes;

  jData["volume_finder"] = DetectorVolumeFinderJsonConverter::toJson(
      detector.detectorVolumeFinder());

  // Write the header
  nlohmann::json jHeader;
  jHeader["detector"] = detector.name();
  jHeader["type"] = "acts";
  jHeader["date"] = std::asctime(ti);
  jHeader["surface_count"] = nSurfaces;
  jHeader["portal_count"] = portals.size();
  jHeader["volume_count"] = detector.volumes().size();
  jDetector["header"] = jHeader;
  jDetector["data"] = jData;
  return jDetector;
}

nlohmann::json Acts::DetectorJsonConverter::toJsonDetray(
    const GeometryContext& gctx, const Experimental::Detector& detector,
    const Options& options) {
  // Get the time stamp
  std::time_t tt = 0;
  std::time(&tt);
  auto ti = std::localtime(&tt);

  nlohmann::json jFile;

  // (1) Detector section
  nlohmann::json jGeometry;
  nlohmann::json jGeometryData;
  nlohmann::json jGeometryHeader;
  std::size_t nSurfaces = 0;

  nlohmann::json jCommonHeader;
  jCommonHeader["detector"] = detector.name();
  jCommonHeader["date"] = std::asctime(ti);
  jCommonHeader["version"] = "detray - 0.44.0";
  jCommonHeader["tag"] = "geometry";

  auto volumes = detector.volumes();

  // Convert the volumes
  nlohmann::json jVolumes;
  for (const auto& volume : volumes) {
    auto jVolume = DetectorVolumeJsonConverter::toJsonDetray(
        gctx, *volume, volumes, options.volumeOptions);
    jVolumes.push_back(jVolume);
    if (jVolume.find("surfaces") != jVolume.end() &&
        jVolume["surfaces"].is_array()) {
      nSurfaces += jVolume["surfaces"].size();
    }
  }
  jGeometryData["volumes"] = jVolumes;
  jGeometryData["volume_grid"] = DetectorVolumeFinderJsonConverter::toJson(
      detector.detectorVolumeFinder(), true);

  // (1) Geometry
  // For detray, the number of surfaces and portals are collected
  // from the translated volumes
  jGeometryHeader["type"] = "detray";
  jGeometryHeader["common"] = jCommonHeader;
  jGeometryHeader["surface_count"] = nSurfaces;
  jGeometryHeader["volume_count"] = detector.volumes().size();
  jGeometry["header"] = jGeometryHeader;
  jGeometry["data"] = jGeometryData;
  jFile["geometry"] = jGeometry;

  // (2) surface grid section
  nlohmann::json jSurfaceGrids;
  nlohmann::json jSurfaceGridsData;
  nlohmann::json jSurfaceGridsInfoCollection;
  nlohmann::json jSurfaceGridsHeader;
  for (const auto [iv, volume] : enumerate(volumes)) {
    // And its surface navigation delegates
    nlohmann::json jSurfacesDelegate = IndexedSurfacesJsonConverter::toJson(
        volume->surfaceCandidatesUpdater(), true);
    if (jSurfacesDelegate.is_null()) {
      continue;
    }
    // Colplete the grid json for detray usage
    jSurfacesDelegate["owner_link"] = iv;
    // jSurfacesDelegate["acc_link"] =
    nlohmann::json jSurfaceGridsCollection;
    jSurfaceGridsCollection.push_back(jSurfacesDelegate);

    nlohmann::json jSurfaceGridsInfo;
    jSurfaceGridsInfo["volume_link"] = iv;
    jSurfaceGridsInfo["grid_data"] = jSurfaceGridsCollection;
    jSurfaceGridsInfoCollection.push_back(jSurfaceGridsInfo);
  }
  jSurfaceGridsData["grids"] = jSurfaceGridsInfoCollection;

  jCommonHeader["tag"] = "surface_grids";
  jSurfaceGridsHeader["common"] = jCommonHeader;
  jSurfaceGridsHeader["grid_count"] = jSurfaceGridsInfoCollection.size();

  jSurfaceGrids["header"] = jSurfaceGridsHeader;
  jSurfaceGrids["data"] = jSurfaceGridsData;

  jFile["surface_grids"] = jSurfaceGrids;

  // (3) material section
  jCommonHeader["tag"] = "material_maps";

  nlohmann::json jMaterial;
  nlohmann::json jMaterialHeader;
  nlohmann::json jMaterialData;
  jMaterialHeader["common"] = jCommonHeader;

  // Shared across all volumes
  nlohmann::json jPhiAxis;
  jPhiAxis["bounds"] = 2;
  jPhiAxis["binning"] = 0;
  jPhiAxis["bins"] = 1;
  jPhiAxis["edges"] = std::array<double, 2>{-M_PI, M_PI};
  // Non-azimutal axis
  nlohmann::json jNonAzimutalAxis;
  jNonAzimutalAxis["bounds"] = 1;
  jNonAzimutalAxis["binning"] = 0;

  nlohmann::json jMaterialGrids;
  std::size_t nGrids = 0;
  for (const auto [iv, volume] : enumerate(volumes)) {

    nlohmann::json jMaterialVolumeGrids;
    jMaterialVolumeGrids["volume_link"] = iv;

    // Create the material json
    int gridIndexInVoume = 0;
    nlohmann::json jMaterialVolumeGridsData;
    for (const auto [is, surface] : enumerate(volume->surfaces())) {
      if (auto binnedMaterial = dynamic_cast<const BinnedSurfaceMaterial*>(
              surface->surfaceMaterial());
          binnedMaterial != nullptr) {
        // Get the bin unitility
        const BinUtility& bUtility = binnedMaterial->binUtility();
        const auto& bDataVec = bUtility.binningData();

        if (bDataVec.size() == 1u) {
          nlohmann::json jMaterialGrid;
          // Create the grid json
          ++nGrids;
          nlohmann::json jMaterialGridData;

          // Grab the binning data
          const BinningData& bData = bDataVec.front();
          Acts::BinningValue bValue = bData.binvalue;

          // Grid link information
          nlohmann::json jGridLink;
          int gridIndexType = bValue == binZ ? 3 : 0;
          jGridLink["type"] = gridIndexType;
          jGridLink["index"] = gridIndexInVoume++;
          jMaterialGridData["grid_link"] = jGridLink;

          // The number of bins
          jNonAzimutalAxis["bins"] = bData.bins();

          // The axes
          if (gridIndexType == 0) {
            jPhiAxis["label"] = 1;
            jNonAzimutalAxis["label"] = 0;
            jNonAzimutalAxis["edges"] =
                std::array<double, 2>{bData.min, bData.max};
            jMaterialGridData["axes"] = {jNonAzimutalAxis, jPhiAxis};
          } else {
            // Concentric cylinder is used, we apply an offset in z
            ActsScalar zOffset = surface->center(Acts::GeometryContext{}).z();
            jNonAzimutalAxis["edges"] =
                std::array<double, 2>{bData.min + zOffset, bData.max + zOffset};
            jPhiAxis["label"] = 0;
            jNonAzimutalAxis["label"] = 1;
            jMaterialGridData["axes"] = {jPhiAxis, jNonAzimutalAxis};
          }
          // Owner link
          jMaterialGridData["owner_link"] = is;

          // The bins
          nlohmann::json jBins;
          auto materialMatrix = binnedMaterial->fullMaterial();
          for (unsigned int ib = 0; ib < bData.bins(); ++ib) {
            nlohmann::json jBin;
            // Local index
            unsigned int b0 = gridIndexType == 0 ? ib : 0;
            unsigned int b1 = gridIndexType == 0 ? 0 : ib;
            jBin["loc_index"] = std::array<unsigned int, 2u>{b0, b1};
            MaterialSlab slab = materialMatrix[0][ib];
            const Material& material = slab.material();
            // The content
            nlohmann::json jContent;
            // consists of thickness
            jContent["thickness"] = slab.thickness();
            // and the actual material
            nlohmann::json jMaterialParams;
            if (slab.thickness() > 0.) {
              jMaterialParams["params"] = std::vector<double>{
                  material.X0(),          material.L0(),          material.Ar(),
                  material.Z(),           material.massDensity(), 0,
                  material.molarDensity()};
            } else {
              jMaterialParams["params"] =
                  std::vector<double>{0., 0., 0., 0., 0., 0., 0.};
            }
            jContent["material"] = jMaterialParams;
            jContent["type"]  = 6;
            jContent["surface_idx"] = is;
            nlohmann::json jContentVector;
            jContentVector.push_back(jContent);
            jBin["content"] = jContentVector;
            jBins.push_back(jBin);
          }
          // Add the actual material bins
          jMaterialGridData["bins"] = jBins;
          // Add the data to the grid
          jMaterialVolumeGridsData.push_back(jMaterialGridData);
        }
      }
    }
    if (!jMaterialVolumeGridsData.empty()) {
      jMaterialVolumeGrids["grid_data"] = { jMaterialVolumeGridsData };
      jMaterialGrids.push_back(jMaterialVolumeGrids);
    }
  }
  jMaterialData["grids"] = jMaterialGrids;
  // Fill the header
  jMaterialHeader["grid_count"] = nGrids;
  jMaterial["header"] = jMaterialHeader;
  jMaterial["data"] = jMaterialData;
  jFile["material"] = jMaterial;

  return jFile;
}

std::shared_ptr<Acts::Experimental::Detector>
Acts::DetectorJsonConverter::fromJson(const GeometryContext& gctx,
                                      const nlohmann::json& jDetector) {
  // Read back all the data
  auto jData = jDetector["data"];
  auto jVolumes = jData["volumes"];
  auto jPortals = jData["portals"];
  const std::string name = jData["name"];

  std::vector<std::shared_ptr<Experimental::DetectorVolume>> volumes;
  std::vector<std::shared_ptr<Experimental::Portal>> portals;

  for (const auto& jVolume : jVolumes) {
    auto volume = DetectorVolumeJsonConverter::fromJson(gctx, jVolume);
    volumes.push_back(volume);
  }

  for (const auto& jPortal : jPortals) {
    auto portal = PortalJsonConverter::fromJson(gctx, jPortal, volumes);
    portals.push_back(portal);
  }

  // Patch all the portals of the volumes
  for (auto [iv, v] : enumerate(volumes)) {
    // Get the Volumes and update the portals with the loaded ones
    auto jVolume = jVolumes[iv];
    std::vector<std::size_t> portalLinks = jVolume["portal_links"];
    for (auto [ip, ipl] : enumerate(portalLinks)) {
      auto portal = portals[ipl];
      v->updatePortal(portal, ip);
    }
  }
  return Experimental::Detector::makeShared(name, volumes,
                                            Experimental::tryRootVolumes());
}
