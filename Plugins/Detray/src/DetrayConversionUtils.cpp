// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsPlugins/Detray/DetrayConversionUtils.hpp"

#include <limits>
#include <numbers>
#include <stdexcept>

using namespace Acts;

detray::axis::label ActsPlugins::DetrayConversionUtils::convertAxisDirection(
    AxisDirection bValue) {
  switch (bValue) {
    case AxisDirection::AxisX:
      return detray::axis::label::e_x;
    case AxisDirection::AxisY:
      return detray::axis::label::e_y;
    case AxisDirection::AxisZ:
      return detray::axis::label::e_z;
    case AxisDirection::AxisR:
      return detray::axis::label::e_r;
    case AxisDirection::AxisPhi:
      return detray::axis::label::e_phi;
    case AxisDirection::AxisRPhi:
      return detray::axis::label::e_rphi;
    default:
      throw std::invalid_argument(
          "DetrayMaterialConverter: unknown binning value detected.");
  }
}

detray::axis::bounds ActsPlugins::DetrayConversionUtils::convertBinningOption(
    BinningOption bOption) {
  // That's a bit of a mind bender, but the conversion is correct
  // closed -> axis are closed, i.e. circular
  // open -> axis are not closed, but the range is closed (no overflow bin) ->
  // closed
  switch (bOption) {
    case BinningOption::closed:
      return detray::axis::bounds::e_circular;
    case BinningOption::open:
      return detray::axis::bounds::e_closed;
    default:
      throw std::invalid_argument(
          "DetrayMaterialConverter: unknown binning option detected.");
  }
}

detray::axis::binning ActsPlugins::DetrayConversionUtils::convertBinningType(
    BinningType bType) {
  switch (bType) {
    case BinningType::equidistant:
      return detray::axis::binning::e_regular;
    case BinningType::arbitrary:
      return detray::axis::binning::e_irregular;
    default:
      throw std::invalid_argument(
          "DetrayMaterialConverter: unknown binning type detected.");
  }
}

detray::io::axis_payload ActsPlugins::DetrayConversionUtils::convertBinningData(
    const BinningData& bData) {
  detray::io::axis_payload axis;

  axis.bins = bData.bins();
  // Set the binning type
  axis.binning = convertBinningType(bData.type);
  // Set the binning option
  axis.bounds = convertBinningOption(bData.option);
  // Set the binning value
  axis.label = convertAxisDirection(bData.binvalue);
  // Set the binning range
  axis.edges = {};
  if (bData.type == BinningType::equidistant) {
    axis.edges = {bData.min, bData.max};
  } else {
    axis.edges.insert(axis.edges.end(), bData.boundaries().begin(),
                      bData.boundaries().end());
  }
  return axis;
}

detray::io::axis_payload ActsPlugins::DetrayConversionUtils::convertAxis(
    const Acts::IAxis& axis) {
  using enum detray::axis::binning;
  detray::io::axis_payload payload;
  payload.bins = axis.getNBins();
  if (axis.isEquidistant()) {
    payload.binning = e_regular;
    payload.edges = {axis.getMin(), axis.getMax()};
  } else {
    payload.binning = e_irregular;
    payload.edges = axis.getBinEdges();
  }

  switch (axis.getBoundaryType()) {
    using enum Acts::AxisBoundaryType;
    case Open:
      payload.bounds = detray::axis::bounds::e_open;
      break;
    case Closed:
      payload.bounds = detray::axis::bounds::e_circular;
      break;
    case Bound:
      payload.bounds = detray::axis::bounds::e_closed;
      break;
  }

  return payload;
}

detray::io::axis_payload
ActsPlugins::DetrayConversionUtils::convertDirectedProtoAxis(
    const Acts::DirectedProtoAxis& dpa) {
  auto axisPayload = convertAxis(dpa.getAxis());
  axisPayload.label = convertAxisDirection(dpa.getAxisDirection());
  return axisPayload;
}

detray::io::material_slab_payload
ActsPlugins::DetrayConversionUtils::convertMaterialSlab(
    const Acts::MaterialSlab& slab) {
  detray::io::material_slab_payload payload;
  // Fill the material parameters and the thickness
  const auto& material = slab.material();
  payload.thickness = slab.thickness();
  payload.mat = detray::io::material_payload{
      {material.X0(), material.L0(), material.Ar(), material.Z(),
       material.massDensity(), material.molarDensity(), 0.}};
  payload.type = detray::io::material_id::slab;
  return payload;
}

detray::io::transform_payload
ActsPlugins::DetrayConversionUtils::convertTransform(
    const Acts::Transform3& transform) {
  detray::io::transform_payload tfPayload;

  Eigen::Map<Acts::Vector3> tr{tfPayload.tr.data()};
  tr = transform.translation();

  Eigen::Map<Acts::SquareMatrix3> rot{tfPayload.rot.data()};
  rot = transform.linear();

  return tfPayload;
}

std::tuple<Acts::DirectedProtoAxis, Acts::DirectedProtoAxis, bool>
ActsPlugins::DetrayConversionUtils::convertDirectedProtoAxesTo2D(
    const std::vector<Acts::DirectedProtoAxis>& dProtoAxes) {
  using enum Acts::AxisDirection;
  using enum Acts::AxisBoundaryType;

  // Return as-is if already 2D
  if (dProtoAxes.size() == 2u) {
    // Check if we need to swap for phi-z -> z-phi
    if (dProtoAxes[0].getAxisDirection() == AxisZ &&
        dProtoAxes[1].getAxisDirection() == AxisPhi) {
      return {dProtoAxes[1], dProtoAxes[0], true};
    }
    return {dProtoAxes[0], dProtoAxes[1], false};
  }

  // Convert 1D to 2D
  if (dProtoAxes.size() == 1u) {
    const DirectedProtoAxis& dpa0 = dProtoAxes[0];
    if (dpa0.getAxisDirection() == AxisX) {
      DirectedProtoAxis dpa1(AxisY, Bound, std::numeric_limits<float>::lowest(),
                             std::numeric_limits<float>::max(), 1u);
      return {dpa0, dpa1, false};
    } else if (dpa0.getAxisDirection() == AxisY) {
      // Turn to X-Y (swap needed)
      DirectedProtoAxis dpa1(AxisX, Bound, std::numeric_limits<float>::lowest(),
                             std::numeric_limits<float>::max(), 1u);
      return {dpa1, dpa0, true};
    } else if (dpa0.getAxisDirection() == AxisR) {
      // Turn to R-Phi
      DirectedProtoAxis dpa1(AxisPhi, Closed, -std::numbers::pi,
                             std::numbers::pi, 1u);
      return {dpa0, dpa1, false};
    } else if (dpa0.getAxisDirection() == AxisZ) {
      // Turn to Phi-Z (swap needed)
      DirectedProtoAxis dpa1(AxisPhi, Closed, -std::numbers::pi,
                             std::numbers::pi, 1u);
      return {dpa1, dpa0, true};
    } else if (dpa0.getAxisDirection() == AxisPhi) {
      // Turn to R-Phi
      DirectedProtoAxis dpa1(AxisR, Bound, 0.0f,
                             std::numeric_limits<float>::max(), 1u);
      return {dpa0, dpa1, false};
    }
  }

  throw std::invalid_argument("Unsupported binning for Detray");
}
