// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <map>

#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/detail/RealQuadraticEquation.hpp"

#include "ActsFatras/Digitization/PlanarDigitizer.hpp"

std::vector<ActsFatras::DigitizationCell>
ActsFatras::PlanarDigitizer::cellsLocal(
    const Acts::BinUtility& bUtility, const Acts::Vector2D& start2D,
    const Acts::Vector2D& end2D,
    std::function<double(const Eigen::ParametrizedLine<double, 2>&,
                         unsigned int, float)>
        stepper) const {
  std::vector<ActsFatras::DigitizationCell> cells = {};

  std::map<double, std::pair<int, unsigned int>> walk;
  auto digiLine = Eigen::ParametrizedLine<double, 2>::Through(start2D, end2D);
  double digiLength = (end2D - start2D).norm();

  const auto& bData = bUtility.binningData();

  /// Helper method to walk along the ib direction and step bin boundaries
  ///
  /// @param ib is the current bin
  ///
  /// @return the start bin & end bin
  auto stepAlong =
      [&](unsigned int ib) -> std::pair<unsigned int, unsigned int> {
    auto startBin = bUtility.bin(start2D, ib);
    auto endBin = bUtility.bin(end2D, ib);

    // Fast exit for staying within the bin
    if (startBin == endBin) {
      walk.insert({digiLength, {0, ib}});
    } else {
      int binStep = startBin < endBin ? 1 : -1;
      const auto& binData = bData[ib];

      auto clippedBoundaries =
          clip(binData.boundaries(), startBin - binStep, endBin + binStep);

      for (auto cbv : clippedBoundaries) {
        auto stepLength = stepper(digiLine, ib, cbv);
        if (stepLength > 0. and stepLength < digiLength) {
          walk.insert({stepLength, {binStep, ib}});
        }
      }
    }
    return {startBin, endBin};
  };

  // Now perform the walk
  std::pair<unsigned int, unsigned int> gridPoint;
  std::array<std::pair<unsigned int, unsigned int>, 2> grid = {gridPoint,
                                                               gridPoint};
  for (unsigned int id = 0; id < bUtility.dimensions(); ++id) {
    grid[id] = stepAlong(id);
  }

  float walked = 0.;
  cells.reserve(walk.size());
  for (const auto& step : walk) {
    cells.push_back(ActsFatras::DigitizationCell{
        grid[0].first, grid[1].first, float(step.first - walked), 0.});
    grid[step.second.second].first += step.second.first;
    walked = step.first;
  }
  // Add the final cell
  cells.push_back(ActsFatras::DigitizationCell{grid[0].second, grid[1].second,
                                               float(digiLength - walked), 0.});

  return cells;
}

std::vector<ActsFatras::DigitizationCell> ActsFatras::PlanarDigitizer::cells(
    const Acts::GeometryContext& gctx, const Acts::Vector3D& start,
    const Acts::Vector3D& end, const Acts::Surface& sf,
    const Acts::BinUtility& bUtility, const Acts::Vector3D& drift) const {
  // This only works for PlaneSurface and DiscSurface
  auto surfaceType = sf.type();
  if (surfaceType != Acts::Surface::Plane and
      surfaceType != Acts::Surface::Disc) {
    return {};
  }

  using Plane = Eigen::Hyperplane<double, 3>;
  using Line = Eigen::ParametrizedLine<double, 3>;

  // Determine the direction of the drift, constant drift velocity assumed
  auto sfn = sf.normal(gctx);
  auto sfc = sf.center(gctx);

  // The readout plane and the projected impacts into the plane
  auto readoutPlane = Plane::Through(sfn, sfc);

  auto sLine = Line::Through(start, start + drift);
  Acts::Vector3D pStart = start + sLine.intersection(readoutPlane) * drift;

  auto eLine = Line::Through(end, end + drift);
  Acts::Vector3D pEnd = start + eLine.intersection(readoutPlane) * drift;

  auto toLocal = sf.transform(gctx).inverse();
  Acts::Vector2D start2D = (toLocal * pStart).block<2, 1>(0, 0);
  Acts::Vector2D end2D = (toLocal * pEnd).block<2, 1>(0, 0);

  // If necessary constrain to surface bounds
  auto localPath = mask(gctx, start2D, end2D, sf);

  if (localPath == std::nullopt) {
    return {};
  }

  if (surfaceType == Acts::Surface::Plane) {
    /// Cartesian (x-y) grid stepper
    ///
    /// @param dLine The digitisation direction
    /// @param ib Towards the next ib boundary
    /// @param bValue The value of the next ib boundary
    ///
    /// @return distance to next ib boundary
    auto cartesianStepper = [](const Eigen::ParametrizedLine<double, 2>& dLine,
                               unsigned int ib, float bValue) -> double {
      Acts::Vector2D n(0., 0.);
      n[ib] = 1.;
      return dLine.intersection(Eigen::Hyperplane<double, 2>(n, bValue * n));
    };
    return cellsLocal(bUtility, localPath->first, localPath->second,
                      cartesianStepper);
  }

  /// Polar (r-Phi) grid stepper
  ///
  /// @param dLine The digitisation direction
  /// @param ib Towards the next ib boundary
  /// @param bValue The value of the next ib boundary
  ///
  /// @return distance to next ib boundary
  auto polarStepper = [](const Eigen::ParametrizedLine<double, 2>& dLine,
                         unsigned int ib, float bValue) -> double {
    // Stepping in through r boundaries
    if (ib == 0) {
      double k = dLine.direction().y();
      double d = dLine.origin().y() - k * dLine.origin().x();
      Acts::detail::RealQuadraticEquation solver((1 + k * k), (2 * k * d),
                                                 (d * d - bValue * bValue));
      double y0 = k * solver.first + d;
      Acts::Vector2D toSol0 = Acts::Vector2D(solver.first, y0) - dLine.origin();
      double dist0 =
          std::copysign(toSol0.norm(), toSol0.dot(dLine.direction()));
      if (solver.solutions <= 1) {
        return dist0;
      }
      double y1 = k * solver.second + d;
      Acts::Vector2D toSol1 =
          Acts::Vector2D(solver.second, y1) - dLine.origin();
      double dist1 =
          std::copysign(toSol1.norm(), toSol1.dot(dLine.direction()));
      if (dist1 * dist0 < 0.) {
        return (dist1 > 0.) ? dist1 : dist0;
      }
      return (dist1 * dist1 < dist0 * dist0) ? dist1 : dist0;
    }
    // Stepping in phi boundaries
    Acts::Vector2D o(0., 0.);
    Acts::Vector2D n(std::sin(bValue), -std::cos(bValue));
    return dLine.intersection(Eigen::Hyperplane<double, 2>(n, o));
  };

  return cellsLocal(bUtility, localPath->first, localPath->second,
                    polarStepper);
}

std::vector<float> ActsFatras::PlanarDigitizer::clip(
    const std::vector<float>& boundaries, unsigned int bs,
    unsigned int be) const {
  if (bs < be) {
    return std::vector(boundaries.cbegin() + bs, boundaries.cbegin() + be);
  }
  auto bsize = boundaries.size();
  return std::vector(boundaries.crend() - bs, boundaries.crend() - be);
}

std::optional<std::pair<Acts::Vector2D, Acts::Vector2D>>
ActsFatras::PlanarDigitizer::mask(const Acts::GeometryContext& gctx,
                                  const Acts::Vector2D& start,
                                  const Acts::Vector2D& end,
                                  const Acts::Surface& sf) const {
  auto surfaceType = sf.type();
  std::pair<Acts::Vector2D, Acts::Vector2D> startEnd(start, end);
  if (surfaceType == Acts::Surface::Plane) {
    /// Internal helper function to maks with a PlanarBounds object
    auto maskByPlane = [](Acts::Vector2D& outside, const Acts::Vector2D& inside,
                          const Acts::SurfaceBounds& sfBounds) -> void {
      const Acts::PlanarBounds* pBounds =
          dynamic_cast<const Acts::PlanarBounds*>(&sfBounds);
      if (pBounds != nullptr) {
        // Get the vertices
        const auto& pVertices = pBounds->vertices(1);
        auto inwards =
            Eigen::ParametrizedLine<double, 2>::Through(outside, inside);
        for (size_t iv = 0; iv < pVertices.size(); ++iv) {
          const Acts::Vector2D& current = pVertices[iv];
          const Acts::Vector2D& next =
              (iv + 1) < pVertices.size() ? pVertices[iv + 1] : pVertices[0];
          const Acts::Vector2D segment((next - current).normalized());
          const Acts::Vector2D n(segment.y(), -segment.x());
          auto d = inwards.intersection(
              Eigen::Hyperplane<double, 2>(n, pVertices[iv]));
          if (d > 0 and d < (inside - outside).norm()) {
            outside = outside + d * inwards.direction();
          }
        }
      }
      return;
    };

    bool startInside = sf.bounds().inside(start, true);
    bool endInside = sf.bounds().inside(end, true);
    if (not startInside and not endInside) {
      return std::nullopt;
    } else if (not startInside) {
      maskByPlane(startEnd.first, end, sf.bounds());
    } else if (not endInside) {
      maskByPlane(startEnd.second, start, sf.bounds());
    }
  } else {
    /// Internal helper function to maks with a Disc surface object
    auto maskByDisc = [](Acts::Vector2D& outside, const Acts::Vector2D& inside,
                         const Acts::SurfaceBounds& sfBounds) -> void {
      if (sfBounds.type() == Acts::SurfaceBounds::eDisc) {
      } else if (sfBounds.type() == Acts::SurfaceBounds::eDiscTrapezoid) {
      } else if (sfBounds.type() == Acts::SurfaceBounds::eAnnulus) {
      }

      return;
    };

    // Disc like surfaces
    Acts::Vector2D startPolar(start.norm(), Acts::VectorHelpers::phi(start));
    Acts::Vector2D endPolar(end.norm(), Acts::VectorHelpers::phi(end));
    bool startInside = sf.bounds().inside(startPolar, true);
    bool endInside = sf.bounds().inside(endPolar, true);
    if (not startInside and not endInside) {
      return std::nullopt;
    } else if (not startInside) {
      maskByDisc(startEnd.first, end, sf.bounds());
    } else if (not endInside) {
      maskByDisc(startEnd.first, end, sf.bounds());
    }
  }

  return std::make_optional<std::pair<Acts::Vector2D, Acts::Vector2D>>(
      startEnd);
}
