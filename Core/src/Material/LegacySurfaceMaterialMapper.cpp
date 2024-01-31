// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Material/LegacySurfaceMaterialMapper.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Propagator/AbortList.hpp"
#include "Acts/Propagator/ActionList.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Propagator/VolumeCollector.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Surfaces/SurfaceExtractor.hpp"
#include "Acts/Utilities/BinAdjustment.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {
struct EndOfWorldReached;
}  // namespace Acts

Acts::LegacySurfaceMaterialMapper::LegacySurfaceMaterialMapper(
    const Config& cfg, StraightLinePropagator propagator,
    std::unique_ptr<const Logger> slogger)
    : m_cfg(cfg),
      m_propagator(std::move(propagator)),
      m_logger(std::move(slogger)) {
  if (m_cfg.trackingGeometry == nullptr) {
    throw std::invalid_argument("Missing tracking geometry.");
  }
}

std::unique_ptr<Acts::IMaterialMapper::State>
Acts::LegacySurfaceMaterialMapper::createState() const {
  // Parse the geometry and find all surfaces with material proxies
  auto world = m_cfg.trackingGeometry->highestTrackingVolume();

  // The Surface material mapping state
  auto mState = std::make_unique<State>();
  resolveMaterialSurfaces(*mState, *world);
  collectMaterialVolumes(*mState, *world);

  ACTS_DEBUG(mState->accumulatedMaterial.size()
             << " Surfaces with PROXIES collected ... ");
  for (auto& smg : mState->accumulatedMaterial) {
    ACTS_VERBOSE(" -> Surface in with id " << smg.first);
  }
  return mState;
}

void Acts::LegacySurfaceMaterialMapper::resolveMaterialSurfaces(
    State& mState, const TrackingVolume& tVolume) const {
  SurfaceExtractor extractor;
  tVolume.visitSurfaces(extractor, false);

  ACTS_DEBUG("Found " << extractor.extractedSurfaces.size()
                      << " surfaces with material proxies.");

  for (const auto& s : extractor.extractedSurfaces) {
    mState.inputSurfaceMaterial[s->geometryId()] =
        s->surfaceMaterialSharedPtr();
    checkAndInsert(mState, *s);
  }
}

void Acts::LegacySurfaceMaterialMapper::checkAndInsert(State& mState,
                                                 const Surface& surface) const {
  auto surfaceMaterial = surface.surfaceMaterial();
  // Check if the surface has a proxy
  if (surfaceMaterial != nullptr) {
    if (m_cfg.computeVariance) {
      mState.inputSurfaceMaterial[surface.geometryId()] =
          surface.surfaceMaterialSharedPtr();
    }
    auto geoID = surface.geometryId();
    std::size_t volumeID = geoID.volume();
    ACTS_DEBUG("Material surface found with volumeID " << volumeID);
    ACTS_DEBUG("       - surfaceID is " << geoID);

    // We need a dynamic_cast to either a surface material proxy or
    // proper surface material
    auto psm = dynamic_cast<const ProtoSurfaceMaterial*>(surfaceMaterial);

    // Get the bin utility: try proxy material first
    const BinUtility* bu = (psm != nullptr) ? (&psm->binning()) : nullptr;
    if (bu != nullptr) {
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - (proto) binning is " << *bu);
      // Now update
      BinUtility buAdjusted = adjustBinUtility(*bu, surface, GeometryContext{});
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - adjusted binning is " << buAdjusted);
      mState.accumulatedMaterial[geoID] =
          AccumulatedSurfaceMaterial(buAdjusted);
      return;
    }

    // Second attempt: binned material
    auto bmp = dynamic_cast<const BinnedSurfaceMaterial*>(surfaceMaterial);
    bu = (bmp != nullptr) ? (&bmp->binUtility()) : nullptr;
    // Creaete a binned type of material
    if (bu != nullptr) {
      // Screen output for Binned Surface material
      ACTS_DEBUG("       - binning is " << *bu);
      mState.accumulatedMaterial[geoID] = AccumulatedSurfaceMaterial(*bu);
    } else {
      // Create a homogeneous type of material
      ACTS_DEBUG("       - this is homogeneous material.");
      mState.accumulatedMaterial[geoID] = AccumulatedSurfaceMaterial();
    }
  }
}

void Acts::LegacySurfaceMaterialMapper::collectMaterialVolumes(
    State& mState, const TrackingVolume& tVolume) const {
  ACTS_VERBOSE("Checking volume '" << tVolume.volumeName()
                                   << "' for material surfaces.")
  ACTS_VERBOSE("- Insert Volume ...");
  if (tVolume.volumeMaterial() != nullptr) {
    mState.volumeMaterial[tVolume.geometryId()] =
        tVolume.volumeMaterialSharedPtr();
  }

  // Step down into the sub volume
  if (tVolume.confinedVolumes()) {
    ACTS_VERBOSE("- Check children volume ...");
    for (auto& sVolume : tVolume.confinedVolumes()->arrayObjects()) {
      // Recursive call
      collectMaterialVolumes(mState, *sVolume);
    }
  }
  if (!tVolume.denseVolumes().empty()) {
    for (auto& sVolume : tVolume.denseVolumes()) {
      // Recursive call
      collectMaterialVolumes(mState, *sVolume);
    }
  }
}

Acts::DetectorMaterialMaps Acts::LegacySurfaceMaterialMapper::finalizeMaps(
    IMaterialMapper::State& imState) const {
  State& mState = static_cast<State&>(imState);

  Acts::DetectorMaterialMaps detectorMaterial;

  // iterate over the map to call the total average
  for (auto& accMaterial : mState.accumulatedMaterial) {
    ACTS_DEBUG("Finalizing map for Surface " << accMaterial.first);
    detectorMaterial.first[accMaterial.first] =
        accMaterial.second.totalAverage();
  }

  return detectorMaterial;
}

std::array<Acts::RecordedMaterialTrack, 2u>
Acts::LegacySurfaceMaterialMapper::mapMaterialTrack(
    IMaterialMapper::State& imState, const GeometryContext& gctx,
    const MagneticFieldContext& mctx,
    const RecordedMaterialTrack& mTrack) const {
  State& mState = static_cast<State&>(imState);

  RecordedMaterialTrack rTrack = mTrack;

  // Retrieve the recorded material from the recorded material track
  auto& rMaterial = rTrack.second.materialInteractions;
  ACTS_VERBOSE("Retrieved " << rMaterial.size()
                            << " recorded material steps to map.")

  // Check if the material interactions are associated with a surface. If yes
  // we simply need to loop over them and accumulate the material
  if (rMaterial.begin()->intersectionID != GeometryIdentifier()) {
    ACTS_VERBOSE(
        "Material surfaces are associated with the material interaction. The "
        "association interaction/surfaces won't be performed again.");
    mapSurfaceInteraction(mState, rMaterial);
    return {rTrack, rTrack};
  }
  ACTS_VERBOSE(
      "Material interactions need to be associated with surfaces. "
      "Collecting all surfaces on the trajectory.");
  mapInteraction(mState, gctx, mctx, rTrack);
  return {rTrack, rTrack};
}
void Acts::LegacySurfaceMaterialMapper::mapInteraction(
    IMaterialMapper::State& imState, const GeometryContext& gctx,
    const MagneticFieldContext& mctx, RecordedMaterialTrack& mTrack) const {
  State& mState = static_cast<State&>(imState);

  // Retrieve the recorded material from the recorded material track
  auto& rMaterial = mTrack.second.materialInteractions;
  std::map<GeometryIdentifier, unsigned int> assignedMaterial;
  using VectorHelpers::makeVector4;
  // Neutral curvilinear parameters
  NeutralCurvilinearTrackParameters start(
      makeVector4(mTrack.first.first, 0), mTrack.first.second,
      1 / mTrack.first.second.norm(), std::nullopt,
      NeutralParticleHypothesis::geantino());

  // Prepare Action list and abort list
  using MaterialSurfaceCollector = SurfaceCollector<MaterialSurface>;
  using MaterialVolumeCollector = VolumeCollector<MaterialVolume>;
  using ActionList =
      ActionList<MaterialSurfaceCollector, MaterialVolumeCollector>;
  using AbortList = AbortList<EndOfWorldReached>;

  PropagatorOptions<ActionList, AbortList> options(gctx, mctx);

  // Now collect the material layers by using the straight line propagator
  const auto& result = m_propagator.propagate(start, options).value();
  auto mcResult = result.get<MaterialSurfaceCollector::result_type>();
  auto mvcResult = result.get<MaterialVolumeCollector::result_type>();

  auto mappingSurfaces = mcResult.collected;
  auto mappingVolumes = mvcResult.collected;

  // These should be mapped onto the mapping surfaces found
  ACTS_VERBOSE("Found     " << mappingSurfaces.size()
                            << " mapping surfaces for this track.");
  ACTS_VERBOSE("Mapping surfaces are :")
  for (auto& mSurface : mappingSurfaces) {
    ACTS_VERBOSE(" - Surface : " << mSurface.surface->geometryId()
                                 << " at position = (" << mSurface.position.x()
                                 << ", " << mSurface.position.y() << ", "
                                 << mSurface.position.z() << ")");
    assignedMaterial[mSurface.surface->geometryId()] = 0;
  }

  // Run the mapping process, i.e. take the recorded material and map it
  // onto the mapping surfaces:
  // - material steps and surfaces are assumed to be ordered along the
  // mapping ray
  //- do not record the material inside a volume with material
  auto rmIter = rMaterial.begin();
  auto sfIter = mappingSurfaces.begin();
  auto volIter = mappingVolumes.begin();

  // Use those to minimize the lookup
  GeometryIdentifier lastID = GeometryIdentifier();
  GeometryIdentifier currentID = GeometryIdentifier();
  Vector3 currentPos(0., 0., 0);
  float currentPathCorrection = 1.;
  auto currentAccMaterial = mState.accumulatedMaterial.end();

  // To remember the bins of this event
  using MapBin =
      std::pair<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>;
  using MaterialBin = std::pair<AccumulatedSurfaceMaterial*,
                                std::shared_ptr<const ISurfaceMaterial>>;
  std::map<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>
      touchedMapBins;
  std::map<AccumulatedSurfaceMaterial*, std::shared_ptr<const ISurfaceMaterial>>
      touchedMaterialBin;
  if (sfIter != mappingSurfaces.end() &&
      sfIter->surface->surfaceMaterial()->mappingType() ==
          Acts::MappingType::PostMapping) {
    ACTS_WARNING(
        "The first mapping surface is a PostMapping one. Some material from "
        "before the PostMapping surface will be mapped onto it ");
  }

  // Assign the recorded ones, break if you hit an end
  while (rmIter != rMaterial.end() && sfIter != mappingSurfaces.end()) {
    // Material not inside current volume
    if (volIter != mappingVolumes.end() &&
        !volIter->volume->inside(rmIter->position)) {
      double distVol = (volIter->position - mTrack.first.first).norm();
      double distMat = (rmIter->position - mTrack.first.first).norm();
      // Material past the entry point to the current volume
      if (distMat - distVol > s_epsilon) {
        // Switch to next material volume
        ++volIter;
        continue;
      }
    }
    /// check if we are inside a material volume
    if (volIter != mappingVolumes.end() &&
        volIter->volume->inside(rmIter->position)) {
      ++rmIter;
      continue;
    }
    // Do we need to switch to next assignment surface ?
    if (sfIter != mappingSurfaces.end() - 1) {
      int mappingType = sfIter->surface->surfaceMaterial()->mappingType();
      int nextMappingType =
          (sfIter + 1)->surface->surfaceMaterial()->mappingType();

      if (mappingType == Acts::MappingType::PreMapping ||
          mappingType == Acts::MappingType::Sensor) {
        // Change surface if the material after the current surface.
        if ((rmIter->position - mTrack.first.first).norm() >
            (sfIter->position - mTrack.first.first).norm()) {
          if (nextMappingType == Acts::MappingType::PostMapping) {
            ACTS_WARNING(
                "PreMapping or Sensor surface followed by PostMapping. Some "
                "material "
                "from before the PostMapping surface will be mapped onto it");
          }
          ++sfIter;
          continue;
        }
      } else if (mappingType == Acts::MappingType::Default ||
                 mappingType == Acts::MappingType::PostMapping) {
        switch (nextMappingType) {
          case Acts::MappingType::PreMapping:
          case Acts::MappingType::Default: {
            // Change surface if the material closest to the next surface.
            if ((rmIter->position - sfIter->position).norm() >
                (rmIter->position - (sfIter + 1)->position).norm()) {
              ++sfIter;
              continue;
            }
            break;
          }
          case Acts::MappingType::PostMapping: {
            // Change surface if the material after the next surface.
            if ((rmIter->position - sfIter->position).norm() >
                ((sfIter + 1)->position - sfIter->position).norm()) {
              ++sfIter;
              continue;
            }
            break;
          }
          case Acts::MappingType::Sensor: {
            // Change surface if the next material after the next surface.
            if ((rmIter == rMaterial.end() - 1) ||
                ((rmIter + 1)->position - sfIter->position).norm() >
                    ((sfIter + 1)->position - sfIter->position).norm()) {
              ++sfIter;
              continue;
            }
            break;
          }
          default: {
            ACTS_ERROR("Incorrect mapping type for the next surface : "
                       << (sfIter + 1)->surface->geometryId());
          }
        }
      } else {
        ACTS_ERROR("Incorrect mapping type for surface : "
                   << sfIter->surface->geometryId());
      }
    }

    // get the current Surface ID
    currentID = sfIter->surface->geometryId();
    // We have work to do: the assignment surface has changed
    if (!(currentID == lastID)) {
      // Let's (re-)assess the information
      lastID = currentID;
      currentPos = (sfIter)->position;
      currentPathCorrection =
          sfIter->surface->pathCorrection(gctx, currentPos, sfIter->direction);
      currentAccMaterial = mState.accumulatedMaterial.find(currentID);
    }
    // Now assign the material for the accumulation process
    auto tBin = currentAccMaterial->second.accumulate(
        currentPos, rmIter->materialSlab, currentPathCorrection);
    if (touchedMapBins.find(&(currentAccMaterial->second)) ==
        touchedMapBins.end()) {
      touchedMapBins.insert(MapBin(&(currentAccMaterial->second), tBin));
    }
    if (m_cfg.computeVariance) {
      touchedMaterialBin[&(currentAccMaterial->second)] =
          mState.inputSurfaceMaterial[currentID];
    }
    ++assignedMaterial[currentID];
    // Update the material interaction with the associated surface and
    // intersection
    rmIter->surface = sfIter->surface;
    rmIter->intersection = sfIter->position;
    rmIter->intersectionID = currentID;
    rmIter->pathCorrection = currentPathCorrection;
    // Switch to next material
    ++rmIter;
  }

  ACTS_VERBOSE("Surfaces have following number of assigned hits :")
  for (auto& [key, value] : assignedMaterial) {
    ACTS_VERBOSE(" + Surface : " << key << " has " << value << " hits.");
  }

  // After mapping this track, average the touched bins
  for (auto tmapBin : touchedMapBins) {
    std::vector<std::array<std::size_t, 3>> trackBins = {tmapBin.second};
    if (m_cfg.computeVariance) {
      tmapBin.first->trackVariance(
          trackBins, touchedMaterialBin[tmapBin.first]->materialSlab(
                         trackBins[0][0], trackBins[0][1]));
    }
    tmapBin.first->trackAverage(trackBins);
  }

  // After mapping this track, average the untouched but intersected bins
  if (m_cfg.emptyBinCorrection) {
    // Use the assignedMaterial map to account for empty hits, i.e.
    // the material surface has been intersected by the mapping ray
    // but no material step was assigned to this surface
    for (auto& mSurface : mappingSurfaces) {
      auto mgID = mSurface.surface->geometryId();
      // Count an empty hit only if the surface does not appear in the
      // list of assigned surfaces
      if (assignedMaterial[mgID] == 0) {
        auto missedMaterial = mState.accumulatedMaterial.find(mgID);
        if (m_cfg.computeVariance) {
          missedMaterial->second.trackVariance(
              mSurface.position,
              mState.inputSurfaceMaterial[currentID]->materialSlab(
                  mSurface.position),
              true);
        }
        missedMaterial->second.trackAverage(mSurface.position, true);

        // Add an empty material hit for future material mapping iteration
        Acts::MaterialInteraction noMaterial;
        noMaterial.surface = mSurface.surface;
        noMaterial.intersection = mSurface.position;
        noMaterial.intersectionID = mgID;
        rMaterial.push_back(noMaterial);
      }
    }
  }
}

void Acts::LegacySurfaceMaterialMapper::mapSurfaceInteraction(
    IMaterialMapper::State& imState,
    std::vector<MaterialInteraction>& rMaterial) const {
  State& mState = static_cast<State&>(imState);

  using MapBin =
      std::pair<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>;
  std::map<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>
      touchedMapBins;
  std::map<AccumulatedSurfaceMaterial*, std::shared_ptr<const ISurfaceMaterial>>
      touchedMaterialBin;

  // Looping over all the material interactions
  auto rmIter = rMaterial.begin();
  while (rmIter != rMaterial.end()) {
    // get the current interaction information
    GeometryIdentifier currentID = rmIter->intersectionID;
    Vector3 currentPos = rmIter->intersection;
    auto currentAccMaterial = mState.accumulatedMaterial.find(currentID);

    // Now assign the material for the accumulation process
    auto tBin = currentAccMaterial->second.accumulate(
        currentPos, rmIter->materialSlab, rmIter->pathCorrection);
    if (touchedMapBins.find(&(currentAccMaterial->second)) ==
        touchedMapBins.end()) {
      touchedMapBins.insert(MapBin(&(currentAccMaterial->second), tBin));
    }
    if (m_cfg.computeVariance) {
      touchedMaterialBin[&(currentAccMaterial->second)] =
          mState.inputSurfaceMaterial[currentID];
    }
    ++rmIter;
  }

  // After mapping this track, average the touched bins
  for (auto tmapBin : touchedMapBins) {
    std::vector<std::array<std::size_t, 3>> trackBins = {tmapBin.second};
    if (m_cfg.computeVariance) {
      tmapBin.first->trackVariance(
          trackBins,
          touchedMaterialBin[tmapBin.first]->materialSlab(trackBins[0][0],
                                                          trackBins[0][1]),
          true);
    }
    // No need to do an extra pass for untouched surfaces they would have been
    // added to the material interaction in the initial mapping
    tmapBin.first->trackAverage(trackBins, true);
  }
}
