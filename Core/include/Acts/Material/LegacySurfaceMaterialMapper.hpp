// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Workaround for building on clang+libstdc++
#include "Acts/Utilities/detail/ReferenceWrapperAnyCompat.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/AccumulatedSurfaceMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/interface/IMaterialMapper.hpp"
#include "Acts/Propagator/MaterialInteractor.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Propagator/VolumeCollector.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace Acts {

class IVolumeMaterial;
class ISurfaceMaterial;
class TrackingGeometry;
struct MaterialInteraction;

/// @brief selector for finding surface
struct MaterialSurface {
  bool operator()(const Surface& sf) const {
    return (sf.surfaceMaterial() != nullptr);
  }
};

/// @brief selector for finding volume
struct MaterialVolume {
  bool operator()(const TrackingVolume& vf) const {
    return (vf.volumeMaterial() != nullptr);
  }
};

/// @brief LegacySurfaceMaterialMapper
///
/// This is the main feature tool to map material information
/// from a 3D geometry onto the TrackingGeometry with its surface
/// material description.
///
/// The process runs as such:
///
///  1) TrackingGeometry is parsed and for each Surface with
///     ProtoSurfaceMaterial a local store is initialized
///     the identification is done hereby through the
///     Surface::GeometryIdentifier
///
///  2) A Cache is generated that is used to keep the filling thread local,
///     the filling is protected with std::mutex
///
///  3) A number of N material tracks is read in, each track has :
///       origin, direction, material steps < position, step length, x0, l0, a,
///       z, rho >
///
///       for each track:
///          surfaces along the origin/direction path are collected
///          the closest material steps are assigned
///
///  4) Each 'hit' bin per event is counted and averaged at the end of the run
///
class LegacySurfaceMaterialMapper final : public IMaterialMapper {
 public:
  using StraightLinePropagator = Propagator<StraightLineStepper, Navigator>;

  /// @struct Config
  ///
  /// Nested Configuration struct for the material mapper
  struct Config {
    std::shared_ptr<const TrackingGeometry> trackingGeometry = nullptr;

    /// Mapping range
    std::array<double, 2> etaRange = {{-6., 6.}};
    /// Correct for empty bins (recommended)
    bool emptyBinCorrection = true;
    /// Mapping output to debug stream
    bool mapperDebugOutput = false;
    /// Compute the variance of each material slab (only if using an input map)
    bool computeVariance = false;
  };

  /// @struct State
  ///
  /// Nested State struct which is used for the mapping prococess
  class State final : public IMaterialMapper::State {
   public:
    State() = default;

    /// The accumulated material per geometry ID
    std::map<GeometryIdentifier, AccumulatedSurfaceMaterial>
        accumulatedMaterial;

    /// The surface material of the input tracking geometry
    std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
        inputSurfaceMaterial;

    /// The volume material of the input tracking geometry
    std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>
        volumeMaterial;
  };

  /// Delete the Default constructor
  LegacySurfaceMaterialMapper() = delete;

  /// Constructor with config object
  ///
  /// @param cfg Configuration struct
  /// @param propagator The straight line propagator
  /// @param slogger The logger
  LegacySurfaceMaterialMapper(const Config& cfg, StraightLinePropagator propagator,
                        std::unique_ptr<const Logger> slogger =
                            getDefaultLogger("LegacySurfaceMaterialMapper",
                                             Logging::INFO));

  /// @brief helper method that creates the cache for the mapping
  std::unique_ptr<IMaterialMapper::State> createState() const;

  /// @brief Method to finalize the maps
  ///
  /// It calls the final run averaging and then transforms
  /// the AccumulatedSurface material class to a surface material
  /// class type
  ///
  /// @param imState the cached object holding the material
  ///
  /// @return a DetectorMaterialMaps object
  DetectorMaterialMaps finalizeMaps(IMaterialMapper::State& mState) const;

  /// Process/map a single track
  ///
  /// @param imState The current state map
  /// @param gctx The geometry context - in case propagation is needed
  /// @param mctx The magnetic field context - in case propagation is needed
  /// @param mTrack The material track to be mapped
  ///
  /// @note the RecordedMaterialSlab of the track are assumed
  /// to be ordered from the starting position along the starting direction
  ///
  /// @returns the unmapped (remaining) material track
  std::array<RecordedMaterialTrack, 2u> mapMaterialTrack(
      IMaterialMapper::State& imState, const GeometryContext& gctx,
      const MagneticFieldContext& mctx,
      const RecordedMaterialTrack& mTrack) const;

  /// Loop through all the material interactions and add them to the
  /// associated surface
  ///
  /// @param mState The current state map
  /// @param gctx The geometry context - in case propagation is needed
  /// @param mctx The magnetic field context - in case propagation is needed
  /// @param mTrack The material track to be mapped
  ///
  void mapInteraction(IMaterialMapper::State& imState,
                      const GeometryContext& gctx,
                      const MagneticFieldContext& mctx,
                      RecordedMaterialTrack& mTrack) const;

  /// Loop through all the material interactions and add them to the
  /// associated surface
  ///
  /// @param mState The current state map
  /// @param rMaterial Vector of all the material interactions that will be mapped
  ///
  /// @note The material interactions are assumed to have an associated surface ID
  void mapSurfaceInteraction(IMaterialMapper::State& imState,
                             std::vector<MaterialInteraction>& rMaterial) const;

 private:
  /// @brief finds all surfaces with ProtoSurfaceMaterial of a volume
  ///
  /// @param mState The state to be filled
  /// @param tVolume is current TrackingVolume
  void resolveMaterialSurfaces(State& mState,
                               const TrackingVolume& tVolume) const;

  /// @brief check and insert
  ///
  /// @param mState is the map to be filled
  /// @param surface is the surface to be checked for a Proxy
  void checkAndInsert(State& mState, const Surface& surface) const;

  /// @brief check and insert
  ///
  /// @param mState is the map to be filled
  /// @param tVolume is the volume collect from
  void collectMaterialVolumes(State& mState,
                              const TrackingVolume& tVolume) const;

  /// Standard logger method
  const Logger& logger() const { return *m_logger; }

  /// The configuration object
  Config m_cfg;

  /// The straight line propagator
  StraightLinePropagator m_propagator;

  /// The logging instance
  std::unique_ptr<const Logger> m_logger;
};
}  // namespace Acts
