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
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Material/MaterialInteraction.hpp"

namespace Acts {
namespace detail {

/// @brief This method assigns the material interactions to the closest surface intersections
/// 
/// @note it relies on the fact tha predictions and the material tracks are ordered in distance
///
/// @param prediction the ordered predictions
/// @param materialInteractions the ordered material interactions on a track
///
/// @note empty hits will be indicated by an empty MaterialInteraction vector
///
/// @return 
std::vector<std::tuple<SurfaceIntersection, std::vector<MaterialInteraction>>> closestOrdered(
    const std::vector<SurfaceIntersection>& prediction,
    const std::vector<Acts::MaterialInteraction>& materialInteractions);

}  // namespace detail
}  // namespace Acts

