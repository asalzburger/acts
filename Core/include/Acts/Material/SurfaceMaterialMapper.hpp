// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/DetectorMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/interface/IMaterialMapper.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <tuple>
#include <vector>

namespace Acts {

/// @brief SurfaceMaterialMapper
///
/// This material mapper is designed to run a single or a sequence
/// of material mappers in a sequential order.
class SurfaceMaterialMapper final : public IMaterialMapper {
 public:
  using MappedUnmapped = std::array<RecordedMaterialTrack, 2u>;

  // @brief Surface intersection prediction
  using Prediction = std::vector<SurfaceIntersection>;

  /// @brief Surface intersection predictor
  using Predicter = std::function<Prediction(const GeometryContext &,
                                             const MagneticFieldContext &,
                                             const Vector3 &, const Vector3 &)>;

  /// @brief An associateor module
  using AssociatedMaterial =
      std::tuple<SurfaceIntersection, std::vector<MaterialInteraction>>;
  using Associater = std::function<std::vector<AssociatedMaterial>(
      const Prediction &, const RecordedMaterialTrack &)>;

  /// @brief Material accumulation
  using Accumulator = std::function<MappedUnmapped(
      const Prediction &, const RecordedMaterialTrack &)>;

  // @brief Material map provider
  using Provider = std::function<SurfaceMaterialMap()>;

  /// @brief nested configuration struct
  struct Config {
    // Default constructor
    Config() = default;

    /// Constructor with predictor
    /// @param predicterIn is the surface intersection predictor
    /// @param associaterIn is the material interaction associator
    Config(Predicter &&predicterIn, Associater &&associaterIn)
        : predicter(std::move(predicterIn)), associater() {}

    /// @brief Constructor with surfaces, will default to
    ///        detail::TryAllSurfacesPredicter
    /// @param surfaces
    ///
    /// @note this will create a default TryAllSurfacesPredicter
    ///       and a closest path length associater
    Config(const std::vector<const Surface *> &surfaces);

    /// Surface intersection predictor
    Predicter predicter;

    /// Associate the material
    Associater associater;
  };

  /// @brief nested state chachine class
  class State : public IMaterialMapper::State {
   public:
    /// Default constructor
    State() = default;

    /// Mapping output statistics
    std::size_t nTracks = 0;
    std::size_t nSteps = 0;
    std::size_t nIntersections = 0;
    std::size_t nAssigned = 0;
    std::size_t nUnassigned = 0;
  };

  /// @brief Constructor
  ///
  /// @param cfg is the configuration struct
  SurfaceMaterialMapper(const Config &cfg,
                        std::unique_ptr<const Logger> logger = getDefaultLogger(
                            "SurfaceMaterialMapper", Acts::Logging::INFO));

  /// @brief Interface method to create a caching state
  ///
  /// Dedicated material mappers can overload this caching
  /// state to store information for the material mapping
  ///
  /// @return a state object
  virtual std::unique_ptr<IMaterialMapper::State> createState() const final;

  /// @brief Interface mtheod to Process/map a single track
  ///
  /// @param mState The current state map
  /// @param gctx The geometry context - in case propagation is needed
  /// @param mctx The magnetic field context - in case propagation is needed
  /// @param mTrack The material track to be mapped
  ///
  /// @note the RecordedMaterialSlab of the track are assumed
  /// to be ordered from the starting position along the starting direction
  ///
  /// @returns the mapped & unmapped material track (in this order)
  virtual std::array<RecordedMaterialTrack, 2u> mapMaterialTrack(
      IMaterialMapper::State &mState, const GeometryContext &gctx,
      const MagneticFieldContext &mctx,
      const RecordedMaterialTrack &mTrack) const final;

  /// @brief Method to finalize the maps
  ///
  /// It calls the final run averaging and then transforms
  /// the recorded and accummulated material into the final
  /// representation
  ///
  /// @param mState The current state map
  ///
  /// @return the final detector material
  virtual DetectorMaterialMaps finalizeMaps(
      IMaterialMapper::State &mState) const final;

 private:
  /// Standard logger method
  const Logger &logger() const { return *m_logger; }

  /// The configuration object
  Config m_cfg;

  /// The logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts
