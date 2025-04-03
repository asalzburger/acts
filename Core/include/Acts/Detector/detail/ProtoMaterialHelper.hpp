// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <tuple>
#include <vector>

namespace Acts {

class Surface;

namespace Experimental::detail::ProtoMaterialHelper {

/// @brief Method that attaches proto material to a surface given
/// a proto binning description
///
/// @param gctx is the geometry context, needed for extent measuring
/// @param surface is the portal where the material is attached
/// @param bDescription is the binning description for the proto binning
///
/// @return an (eventual) updated binning description for structured
///         screen logging output
std::vector<std::tuple<ProtoAxis, AxisDirection>> attachProtoMaterial(
    const GeometryContext& gctx, Surface& surface,
    const std::vector<std::tuple<ProtoAxis, AxisDirection>>& bDescription);

}  // namespace Experimental::detail::ProtoMaterialHelper
}  // namespace Acts
