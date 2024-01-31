// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/interface/IMaterialMapper.hpp"

#include <memory>

namespace Acts {

/// @brief SequentialMaterialMapper
///
/// This material mapper is designed to run a single or a sequence
/// of material mappers in a sequential order.
class SequentialMaterialMapper final : public IMaterialMapper {
 public:
  /// @brief nested configuration struct
  struct Config {
    std::vector<std::shared_ptr<const IMaterialMapper>> mappers = {};
  };

  /// @brief nested state chachine class
  class State : public IMaterialMapper::State {
   public:
    /// Default constructor
    State() = default;
    /// @brief Shorthand for the mapper and state
    using MapperAndState = std::tuple<const IMaterialMapper *,
                                      std::unique_ptr<IMaterialMapper::State>>;
    /// The vector of mappers and states
    std::vector<MapperAndState> mappersAndStates = {};
  };

  /// @brief Constructor
  ///
  /// @param cfg is the configuration struct
  SequentialMaterialMapper(const Config &cfg);

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
  /// The configuration
  Config m_cfg;
};

}  // namespace Acts
