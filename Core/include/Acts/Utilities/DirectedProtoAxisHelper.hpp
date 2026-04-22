// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

#include <vector>

namespace Acts {

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const RadialBounds& rBounds);

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const CylinderBounds& cBounds);

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const RectangleBounds& pBounds);

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const TrapezoidBounds& pBounds);

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes, const Surface& surface,
    const GeometryContext& gctx);

}  // namespace Acts
