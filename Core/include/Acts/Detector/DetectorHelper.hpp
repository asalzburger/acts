// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <limits>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace Acts {

namespace Experimental {

class DetectorVolume;
class Portal;

using ProtoContainer = std::map<unsigned int, std::shared_ptr<Portal>>;

/// @brief Connect detector volumes with a given binning,
/// expects fully harmonized input
///
/// @param gctx The geometry context
/// @param bValues the binning values - for the connection
/// @param volumes the volumes to be connected
/// @param logLevel is the screen logging level
///
/// @returns the proto container surfaces of a Proto container
ProtoContainer connectDetectorVolumes(
    const GeometryContext& gctx, const std::vector<BinningValue>& bValues,
    std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Connect containers with a given binning,
/// expects fully harmonized input
///
/// @param gctx The geometry context
/// @param bValues the binning values - for the connection
/// @param containers the containers to be connected
/// @param logLevel is the screen logging level
///
/// @returns the proto container surfaces of a Proto container
ProtoContainer connectContainers(
    const GeometryContext& gctx, const std::vector<BinningValue>& bValue,
    const std::vector<ProtoContainer>& containers,
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

}  // namespace Experimental
}  // namespace Acts
