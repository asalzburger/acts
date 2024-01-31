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
#include "Acts/Material/DetectorMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"

#include <array>

namespace Acts {

/// @brief Interface for material mappin tools
///
/// Surface and volume based mappers should extend this
/// interface, and implement a suitable State extentsion
class IMaterialMapper {
 public:
  // The state object as a base class that allows to chain
  // material mappers of different type.
  class State {
   public:
    virtual ~State() = default;
  };

  /// @brief Interface method to create a caching state
  ///
  /// Dedicated material mappers can overload this caching
  /// state to store information for the material mapping
  ///
  /// @return a state object
  virtual std::unique_ptr<State> createState() const = 0;

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
      State& mState, const GeometryContext& gctx,
      const MagneticFieldContext& mctx,
      const RecordedMaterialTrack& mTrack) const = 0;

  /// @brief Method to finalize the maps
  ///
  /// It calls the final run averaging and then transforms
  /// the recorded and accummulated material into the final
  /// representation
  ///
  /// @param mState
  ///
  /// @return detector material maps
  virtual DetectorMaterialMaps finalizeMaps(State& mState) const = 0;
};
}  // namespace Acts
