// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <tuple>
#include <utility>

#include "Acts/Utilities/Definitions.hpp"
#include "Acts/Utilities/Intersection.hpp"

namespace Acts {

class PlanarBounds;

namespace detail {

struct IntersectionHelper2D {
  /// Intersect two segments
  ///
  /// @param s0 The Start of the segement
  /// @param s1 The end of the segement
  /// @param origin The Start of intersection line
  /// @param direction The Direction of intersection line
  static Intersection2D intersectSegment(const Vector2D& s0, const Vector2D& s1,
                                         const Vector2D& origin,
                                         const Vector2D& dir);

  /// Intersect ellipses
  ///
  /// @param Rx The radius in x
  /// @param Ry The radius in y
  /// @param origin The Start of intersection line
  /// @param direction The Direction of intersection line
  ///
  /// @return the intersection points and a boolean if successful
  static std::pair<Intersection2D, Intersection2D> intersectEllipse(
      double Rx, double Ry, const Vector2D& origin, const Vector2D& dir);

  /// Intersect the circle
  ///
  /// @param R The radius
  /// @param origin The Start of intersection line
  /// @param direction The Direction of intersection line
  ///
  /// @return the intersection points and a boolean if successful
  static inline std::pair<Intersection2D, Intersection2D> intersectCircle(
      double R, const Vector2D& origin, const Vector2D& dir) {
    return intersectEllipse(R, R, origin, dir);
  }

  ///  Mask a segment with a planar bounds shape
  ///
  /// If both are inside the bounds, no masking is done, everything
  /// extruding the segement is clipped off.
  ///
  /// @param start of the segment
  /// @param end of the segment
  /// @param pBounds the PlanarBounds object
  ///
  /// @return the fraction of the segment that survived and the (new)
  /// start and end points
  static std::tuple<double, Vector2D, Vector2D> mask(
      const Vector2D& start, const Vector2D& end, const PlanarBounds& pBounds);

  ///  Mask a segment with a planar bounds shape represented as a connected set
  ///  of vertices
  ///
  /// If both are inside the bounds, no masking is done, everything
  /// extruding the segement is clipped off.
  ///
  /// @param start of the segment
  /// @param end of the segment
  /// @param pBounds the PlanarBounds object
  ///
  /// @return the fraction of the segment that survived and the (new)
  /// start and end points
  static std::tuple<double, Vector2D, Vector2D> mask(
      const Vector2D& start, const Vector2D& end,
      const std::vector<Vector2D>& vertices);

};  // struct IntersectionHelper2D

}  // namespace detail
}  // namespace Acts
