// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/DirectedProtoAxisHelper.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace Acts {

namespace {

void throwOnVariableBinning(const DirectedProtoAxis& dProtoAxis) {
  if (dProtoAxis.getAxis().getType() == AxisType::Variable) {
    throw std::invalid_argument("Arbitrary binning can not be adjusted.");
  }
}

}  // namespace

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const RadialBounds& rBounds) {
  const double minR = rBounds.get(RadialBounds::eMinR);
  const double maxR = rBounds.get(RadialBounds::eMaxR);
  const double minPhi = rBounds.get(RadialBounds::eAveragePhi) -
                        rBounds.get(RadialBounds::eHalfPhiSector);
  const double maxPhi = rBounds.get(RadialBounds::eAveragePhi) +
                        rBounds.get(RadialBounds::eHalfPhiSector);

  std::vector<DirectedProtoAxis> adjustedAxes = dProtoAxes;
  for (auto& dProtoAxis : adjustedAxes) {
    throwOnVariableBinning(dProtoAxis);
    const AxisDirection axisDirection = dProtoAxis.getAxisDirection();
    if (axisDirection != AxisDirection::AxisR &&
        axisDirection != AxisDirection::AxisPhi) {
      throw std::invalid_argument("Disc binning must be: phi, r");
    }
    if (axisDirection == AxisDirection::AxisPhi) {
      dProtoAxis.setRange(minPhi, maxPhi);
    } else {
      dProtoAxis.setRange(minR, maxR);
    }
  }
  return adjustedAxes;
}

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const CylinderBounds& cBounds) {
  const double cR = cBounds.get(CylinderBounds::eR);
  const double cHz = cBounds.get(CylinderBounds::eHalfLengthZ);
  const double avgPhi = cBounds.get(CylinderBounds::eAveragePhi);
  const double halfPhi = cBounds.get(CylinderBounds::eHalfPhiSector);
  const double minPhi = avgPhi - halfPhi;
  const double maxPhi = avgPhi + halfPhi;

  std::vector<DirectedProtoAxis> adjustedAxes = dProtoAxes;
  for (auto& dProtoAxis : adjustedAxes) {
    throwOnVariableBinning(dProtoAxis);
    const AxisDirection axisDirection = dProtoAxis.getAxisDirection();
    if (axisDirection != AxisDirection::AxisRPhi &&
        axisDirection != AxisDirection::AxisPhi &&
        axisDirection != AxisDirection::AxisZ) {
      throw std::invalid_argument("Cylinder binning must be: rphi, phi, z");
    }
    if (axisDirection == AxisDirection::AxisPhi) {
      dProtoAxis.setRange(minPhi, maxPhi);
    } else if (axisDirection == AxisDirection::AxisRPhi) {
      dProtoAxis.setRange(cR * minPhi, cR * maxPhi);
    } else {
      dProtoAxis.setRange(-cHz, cHz);
    }
  }
  return adjustedAxes;
}

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const RectangleBounds& pBounds) {
  const double minX = pBounds.get(RectangleBounds::eMinX);
  const double minY = pBounds.get(RectangleBounds::eMinY);
  const double maxX = pBounds.get(RectangleBounds::eMaxX);
  const double maxY = pBounds.get(RectangleBounds::eMaxY);

  std::vector<DirectedProtoAxis> adjustedAxes = dProtoAxes;
  for (auto& dProtoAxis : adjustedAxes) {
    throwOnVariableBinning(dProtoAxis);
    const AxisDirection axisDirection = dProtoAxis.getAxisDirection();
    if (axisDirection != AxisDirection::AxisX &&
        axisDirection != AxisDirection::AxisY) {
      throw std::invalid_argument("Rectangle binning must be: x, y. ");
    }
    if (axisDirection == AxisDirection::AxisX) {
      dProtoAxis.setRange(minX, maxX);
    } else {
      dProtoAxis.setRange(minY, maxY);
    }
  }
  return adjustedAxes;
}

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes,
    const TrapezoidBounds& pBounds) {
  const double halfX = std::max(pBounds.get(TrapezoidBounds::eHalfLengthXnegY),
                                pBounds.get(TrapezoidBounds::eHalfLengthXposY));
  const double halfY = pBounds.get(TrapezoidBounds::eHalfLengthY);

  std::vector<DirectedProtoAxis> adjustedAxes = dProtoAxes;
  for (auto& dProtoAxis : adjustedAxes) {
    throwOnVariableBinning(dProtoAxis);
    const AxisDirection axisDirection = dProtoAxis.getAxisDirection();
    if (axisDirection != AxisDirection::AxisX &&
        axisDirection != AxisDirection::AxisY) {
      throw std::invalid_argument("Rectangle binning must be: x, y. ");
    }
    if (axisDirection == AxisDirection::AxisX) {
      dProtoAxis.setRange(-halfX, halfX);
    } else {
      dProtoAxis.setRange(-halfY, halfY);
    }
  }
  return adjustedAxes;
}

std::vector<DirectedProtoAxis> adjustDirectedProtoAxes(
    const std::vector<DirectedProtoAxis>& dProtoAxes, const Surface& surface,
    const GeometryContext& gctx) {
  static_cast<void>(gctx);

  if (auto b = dynamic_cast<const CylinderBounds*>(&(surface.bounds()));
      b != nullptr) {
    return adjustDirectedProtoAxes(dProtoAxes, *b);
  }
  if (auto b = dynamic_cast<const RadialBounds*>(&(surface.bounds()));
      b != nullptr) {
    return adjustDirectedProtoAxes(dProtoAxes, *b);
  }
  if (surface.type() == Surface::Plane) {
    if (auto b = dynamic_cast<const RectangleBounds*>(&(surface.bounds()));
        b != nullptr) {
      return adjustDirectedProtoAxes(dProtoAxes, *b);
    }
    if (auto b = dynamic_cast<const TrapezoidBounds*>(&(surface.bounds()));
        b != nullptr) {
      return adjustDirectedProtoAxes(dProtoAxes, *b);
    }
  }

  std::stringstream ss;
  ss << surface.toStream(GeometryContext::dangerouslyDefaultConstruct());
  throw std::invalid_argument(
      "Bin adjustment not implemented for this surface yet:\n" + ss.str());
}

}  // namespace Acts
