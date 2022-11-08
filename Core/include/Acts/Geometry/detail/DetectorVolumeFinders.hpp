// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/Detector.hpp"
#include "Acts/Geometry/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/NavigationDelegates.hpp"
#include "Acts/Geometry/NavigationState.hpp"
#include "Acts/Geometry/detail/NavigationStateUpdators.hpp"
#include "Acts/Utilities/detail/Axis.hpp"
#include "Acts/Utilities/detail/Grid.hpp"

#include <exception>

namespace Acts {
namespace Experimental {

namespace detail {

/// @brief The end of world sets the volume pointer of the
/// navigation state to nullptr, usually indicates the end of
/// the known world, hence the name
struct TryAndErrorImpl : public IDelegateImpl {
  /// @brief a null volume link - explicitely
  ///
  /// @param gctx the geometry context for this call
  /// @param nState the navigation state into which the volume is set
  inline static void update(const GeometryContext& gctx,
                            NavigationState& nState) {
    if (nState.currentDetector == nullptr) {
      throw std::runtime_error(
          "DetectorVolumeFinders: no detectore set to navigation state.");
    }

    auto volumes = nState.currentDetector->volumes();
    for (const auto v : volumes) {
      if (v->inside(gctx, nState.position)) {
        nState.currentVolume = v;
      }
    }
    nState.currentVolume = nullptr;
  }
};

/// @brief A helper struct that allows to extrace a volume
/// from the detector by its index
struct IndexedVolumeExtractor : public IDelegateImpl {
  /// Extract the surfaces from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the current navigation state
  /// @param index is the index in the global detector volume store
  ///
  /// @return a vector of raw DetectorVolume pointers
  inline static const std::vector<const DetectorVolume*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState, size_t index) {
    if (nState.detector == nullptr) {
      throw std::runtime_error("IndexedVolumeExtractor: no detector given.");
    }
    // Get the volume container from the detector
    const auto& volumes = nState.detector->volumes();
    return volumes[index];
  }
};

}  // namespace detail
}  // namespace Experimental
}  // namespace Acts
