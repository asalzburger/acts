// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cmath>
#include <iostream>
#include <tuple>

#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/detail/IntersectionHelper2D.hpp"
#include "Acts/Utilities/detail/RealQuadraticEquation.hpp"
#include "Acts/Utilities/Helpers.hpp"

Acts::Intersection2D Acts::detail::IntersectionHelper2D::intersectSegment(
    const Vector2D& s0, const Vector2D& s1, const Vector2D& origin,
    const Vector2D& dir, bool segmentCheck) {
  using Line = Eigen::ParametrizedLine<double, 2>;
  using Plane = Eigen::Hyperplane<double, 2>;

  Vector2D ldir(s1 - s0);
  double det = ldir.x() * dir.y() - ldir.y() * dir.x();
  if (std::abs(det) < s_epsilon) {
    return Intersection2D();
  }

  auto line = Line(origin, dir);
  auto d = line.intersectionParameter(Plane::Through(s0, s1));

  Vector2D sol = origin + d * dir;

  Intersection2D::Status status =
      (not segmentCheck or std::abs((sol - s0).norm() + (s1 - sol).norm() -
                                    (s1 - s0).norm()) < s_onSurfaceTolerance)
          ? Intersection2D::Status::reachable
          : Intersection2D::Status::unreachable;

  return Intersection2D(sol, d, Intersection2D::Status::reachable);
}

std::pair<Acts::Intersection2D, Acts::Intersection2D>
Acts::detail::IntersectionHelper2D::intersectEllipse(double Rx, double Ry,
                                                     const Vector2D& origin,
                                                     const Vector2D& dir) {
  auto createSolution = [&](const Vector2D& sol, const Vector2D& alt)
      -> std::pair<Acts::Intersection2D, Acts::Intersection2D> {
    Vector2D toSolD(sol - origin);
    Vector2D toAltD(alt - origin);
    double solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    double altD = std::copysign(toAltD.norm(), toAltD.dot(dir));

    if (solD * solD < altD * altD) {
      return {Intersection2D(sol, solD, Intersection2D::Status::reachable),
              Intersection2D(alt, altD, Intersection2D::Status::reachable)};
    }
    return {Intersection2D(alt, altD, Intersection2D::Status::reachable),
            Intersection2D(sol, solD, Intersection2D::Status::reachable)};
  };

  // Special cases first
  if (std::abs(dir.x()) < s_epsilon) {
    double solx = origin.x();
    double D = 1. - solx * solx / (Rx * Rx);
    if (D > 0.) {
      double sqrtD = std::sqrt(D);
      Vector2D sol(solx, Ry * sqrtD);
      Vector2D alt(solx, -Ry * sqrtD);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2D(solx, 0.), -origin.y(),
                             Intersection2D::Status::reachable),
              Intersection2D()};
    }
    return {Intersection2D(), Intersection2D()};
  } else if (std::abs(dir.y()) < s_epsilon) {
    double soly = origin.y();
    double D = 1. - soly * soly / (Ry * Ry);
    if (D > 0.) {
      double sqrtD = std::sqrt(D);
      Vector2D sol(Rx * sqrtD, soly);
      Vector2D alt(-Rx * sqrtD, soly);
      return createSolution(sol, alt);
    } else if (std::abs(D) < s_epsilon) {
      return {Intersection2D(Vector2D(0., soly), -origin.x(),
                             Intersection2D::Status::reachable),
              Intersection2D()};
    }
    return {Intersection2D(), Intersection2D()};
  }
  // General solution
  double k = dir.y() / dir.x();
  double d = origin.y() - k * origin.x();
  double Ry2 = Ry * Ry;
  double alpha = 1. / (Rx * Rx) + k * k / Ry2;
  double beta = 2. * k * d / Ry2;
  double gamma = d * d / Ry2 - 1;
  Acts::detail::RealQuadraticEquation solver(alpha, beta, gamma);
  if (solver.solutions == 1) {
    double x = solver.first;
    Vector2D sol(x, k * x + d);
    Vector2D toSolD(sol - origin);
    double solD = std::copysign(toSolD.norm(), toSolD.dot(dir));
    return {Intersection2D(sol, solD, Intersection2D::Status::reachable),
            Intersection2D()};
  } else if (solver.solutions > 1) {
    double x0 = solver.first;
    double x1 = solver.second;
    Vector2D sol(x0, k * x0 + d);
    Vector2D alt(x1, k * x1 + d);
    return createSolution(sol, alt);
  }
  return {Intersection2D(), Intersection2D()};
}

std::tuple<double, Acts::Vector2D, Acts::Vector2D>
Acts::detail::IntersectionHelper2D::mask(const Vector2D& start,
                                         const Vector2D& end,
                                         const PlanarBounds& pBounds) {
  bool startInside = pBounds.inside(start, true);
  bool endInside = pBounds.inside(end, true);
  if (startInside and endInside) {
    return {1, start, end};
  } else if (pBounds.type() == SurfaceBounds::eEllipse) {
    auto values = pBounds.values();
    double iRx = values[EllipseBounds::eInnerRx];
    double iRy = values[EllipseBounds::eInnerRy];
    double oRx = values[EllipseBounds::eOuterRx];
    double oRy = values[EllipseBounds::eOuterRy];
    double avgPhi = values[EllipseBounds::eAveragePhi];
    double halfPhi = values[EllipseBounds::eHalfPhiSector];
    return mask(start, startInside, end, endInside, iRx, iRy, oRx, oRy, avgPhi,
                halfPhi);
  }
  return mask(start, startInside, end, endInside, pBounds.vertices(1));
}

std::tuple<double, Acts::Vector2D, Acts::Vector2D>
Acts::detail::IntersectionHelper2D::mask(const Vector2D& start,
                                         const Vector2D& end,
                                         const DiscBounds& dBounds) {

  double startR = VectorHelpers::perp(start);
  double startPhi = VectorHelpers::phi(start);
  double endR = VectorHelpers::perp(end);
  double endPhi = VectorHelpers::phi(end);

  Vector2D startPolar(startR, startPhi);
  Vector2D endPolar(endR, endPhi);

  bool startInside = dBounds.inside(starPolar, true);
  bool endInside = dBounds.inside(endPolar, true);

  // Trapezoidal shape falls back to vertices based solution
  if (dBounds.type() == SurfaceBounds::eDiscTrapezoid){
    if (startInside and endInside){
      return {1., start, end };
    }
    return mask(start,startInside,end,endInside,dBounds.vertices(1));
  }

  // Purely radial bounds
  if (dBounds.type() == SurfaceBounds::eDisc){

    double iR = values[RadialBounds::eMinR];
    double oR = values[RadialBounds::eMaxR];
    double avgPhi = values[RadialBounds::eAveragePhi];
    double halfPhi = values[RadialBounds::eHalfPhiSector];
    return mask(start, startInside, end, endInside, iR, iR, oR, oR, avgPhi,
                halfPhi);
  }
  
  // Annulus bounds object
  if (dBounds.type() == SurfaceBounds::eAnnulus){



  }

  return { 0., start, end };
}


std::tuple<double, Acts::Vector2D, Acts::Vector2D>
Acts::detail::IntersectionHelper2D::mask(
    const Vector2D& start, bool startInside, const Vector2D& end,
    bool endInside, const std::vector<Vector2D>& vertices) {
  return {1., start, end};
}

std::tuple<double, Acts::Vector2D, Acts::Vector2D>
Acts::detail::IntersectionHelper2D::mask(const Vector2D& start,
                                         bool startInside, const Vector2D& end,
                                         bool endInside, double rIx, double rIy,
                                         double rOx, double rOy, double avgPhi,
                                         double halfPhi) {
  return {1., start, end};
}
