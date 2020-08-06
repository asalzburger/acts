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
class DiscBounds;

namespace detail {

struct IntersectionHelper2D {
  /// Intersect two segments
  ///
  /// @param s0 The Start of the segement
  /// @param s1 The end of the segement
  /// @param origin The Start of intersection line
  /// @param direction The Direction of intersection line
  /// @param segmentCheck Require that the point is within s0 and s1
  static Intersection2D intersectSegment(const Vector2D& s0, const Vector2D& s1,
                                         const Vector2D& origin,
                                         const Vector2D& dir,
                                         bool segmentCheck = true);

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


  ///  Mask a segment with a disc like bound shape
  /// 
  /// If both are inside the bounds, no masking is done, everything
  /// extruding the segement is clipped off.
  ///
  /// @param start of the segment
  /// @param end of the segment
  /// @param dBounds the DiscBounds object
  ///
  /// @return the fraction of the segment that survived and the (new)
  /// start and end points
  static std::tuple<double, Vector2D, Vector2D> mask(
      const Vector2D& start, const Vector2D& end, const DiscBounds& dBounds);


  ///  Mask a segment with a planar bounds shape represented as a connected set
  ///  of vertices
  ///
  /// If both are inside the bounds, no masking is done, everything
  /// extruding the segement is clipped off.
  ///
  /// @param start of the segment
  /// @param startInside boolean flag if start is inside
  /// @param end of the segment
  /// @param endInside bollean flag if the end is inside
  /// @param vertices the PlanarBounds represented as vertices
  ///
  /// @return the fraction of the segment that survived and the (new)
  /// start and end points
  static std::tuple<double, Vector2D, Vector2D> mask(
      const Vector2D& start, bool startInside, const Vector2D& end,
      bool endInside, const std::vector<Vector2D>& vertices);

  ///  Mask a segment with a planar ellipsoid bound shape
  ///
  /// If both are inside the bounds, no masking is done, everything
  /// extruding the segement is clipped off.
  ///
  /// @param start of the segment
  /// @param startInside boolean flag if start is inside
  /// @param end of the segment
  /// @param endInside bollean flag if the end is inside
  /// @param rIx The inner radius in X
  /// @param rYx The inner radius in Y
  /// @param rOx The outer radius in X
  /// @param rOy The outer radius in Y
  /// @param avgPhi The central phi value
  /// @param halfPhi The half phi value
  ///
  /// @return the fraction of the segment that survived and the (new)
  /// start and end points
  static std::tuple<double, Vector2D, Vector2D> mask(
      const Vector2D& start, bool startInside, const Vector2D& end,
      bool endInside, double rIx, double rIy, double rOx, double rOy,
      double avgPhi, double halfPhi);

};  // struct IntersectionHelper2D

}  // namespace detail
}  // namespace Acts
