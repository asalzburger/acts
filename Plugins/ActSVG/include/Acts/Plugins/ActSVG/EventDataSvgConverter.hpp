// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Visualization/Interpolation3D.hpp"
#include <actsvg/core.hpp>

namespace Acts::Svg::EventDataConverter {

/// Write/create a 3D point in XY view
///
/// @param pos the position
/// @param size the size of the object
/// @param style the style of the object
/// @param idx the running index
///
/// @return a vector of svg objects
actsvg::svg::object pointXY(const Vector3& pos, double size, const Style& style,
                            unsigned int idx = 0);

/// Write/create a 3D point in ZR view
///
/// @param pos the position
/// @param size the size of the object
/// @param style the style of the object
/// @param indx the running index
///
/// @return a vector of svg objects
actsvg::svg::object pointZR(const Vector3& pos, double size, const Style& style,
                            unsigned int idx = 0);

/// Write/create a 3D point in a given view
///
/// @param pos the position
/// @param size the size of the object
/// @param style the style of the object
/// @param indx the running index
///
/// @return a vector of svg objects
template <typename view_type>
actsvg::svg::object point(const Vector3& pos, double size, const Style& style,
                          unsigned int idx) {
  view_type view;
  std::vector<Vector3> ps = {pos};
  auto ppos = view(ps)[0];
  auto [fill, stroke] = style.fillAndStroke();
  auto circle =
      actsvg::draw::circle("p_" + std::to_string(idx), ppos,
                           static_cast<actsvg::scalar>(size), fill, stroke);
  return circle;
}

/// A trajectory view function
///
/// @tparam trajectory_type the type of the trajectory
/// @tparam view_type the type of the view
///
/// @param traj the trajectory to be drawn
/// @param hitSize the size of the point objects
/// @param style the style of the object
/// @param nInterpolationPoints the number of interpolation points
///
template <typename trajectory_type, typename view_type>
actsvg::svg::object trajectory(const trajectory_type& traj,
                               double hitSize, const Style& style,
                               unsigned int nInterpolationPoints,
                               unsigned int idx) {
  view_type view;

  trajectory_type interpolatedTraj;
  if (nInterpolationPoints > 0) {
    interpolatedTraj = Interpolation3D::spline(
        traj, traj.size() * (1 + nInterpolationPoints) - 1, false);
  } else {
    interpolatedTraj = traj;
  }

  auto trajView = view(interpolatedTraj);
  auto [fill, stroke] = style.fillAndStroke();

  actsvg::svg::object trajObj;
  trajObj._id = "trajectory_" + std::to_string(idx);
  trajObj._tag = "g";
  trajObj.add_object(actsvg::draw::polyline(
      "trajectory_path_" + std::to_string(idx), trajView, stroke));

  if (hitSize > 0.) {
    auto hitView = view(traj);
    for (const auto& p : hitView) {
      auto circle = actsvg::draw::circle(
          "trajectory_point_" + std::to_string(idx), p,
          static_cast<actsvg::scalar>(hitSize), fill, stroke);
      trajObj.add_object(circle);
    }
  }

  return trajObj;
}

template <typename trajectory_type>
actsvg::svg::object trajectoryXY(const trajectory_type& traj,
                                 double hitSize, const Style& style,
                                 unsigned int nInterpolationPoints,
                                 unsigned int idx) {
  return trajectory<trajectory_type, actsvg::views::x_y>(
      traj, hitSize, style, nInterpolationPoints, idx);
}

template <typename trajectory_type>
actsvg::svg::object trajectoryZR(const trajectory_type& traj,
                                 double hitSize, const Style& style,
                                 unsigned int nInterpolationPoints,
                                 unsigned int idx) {
  return trajectory<trajectory_type, actsvg::views::z_r>(
      traj, hitSize, style, nInterpolationPoints, idx);
}

}  // namespace Acts::Svg::EventDataConverter
