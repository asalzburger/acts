// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Visualization/ViewConfig.hpp"
#include <actsvg/meta.hpp>

#include <array>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

namespace Acts::Svg {

/// @brief Style struct
struct Style {
  // Fill parameters
  std::array<int, 3> fillColor = {255, 255, 255};
  double fillOpacity = 1.;

  // Highlight parameters
  std::array<int, 3> highlightColor = {0, 0, 0};
  std::vector<std::string> highlights = {};

  double strokeWidth = 0.5;
  std::array<int, 3> strokeColor = {0, 0, 0};

  double highlightStrokeWidth = 2;
  std::array<int, 3> highlightStrokeColor = {0, 0, 0};

  std::vector<int> strokeDasharray = {};

  unsigned int fontSize = 14u;
  std::array<int, 3> fontColor = {0};

  /// Number of segments to approximate a quarter of a circle
  unsigned int quarterSegments = 72u;

  /// @brief Default constructor
  Style() = default;

  /// @brief Constructor from Acts ViewConfig
  /// @param vConfig
  explicit Style(ViewConfig vConfig);

  /// @brief Explicit contstructor from color and opacity
  /// @param fillColor
  /// @param fillOpacity
  explicit Style(std::array<int, 3> fillColor_, double fillOpacity_ = 1.)
      : fillColor(fillColor_), fillOpacity(fillOpacity_) {}

  /// Conversion to fill and stroke object from the base library
  /// @return a tuple of actsvg digestable objects
  std::tuple<actsvg::style::fill, actsvg::style::stroke> fillAndStroke() const;

  /// Conversion to fill, stroke and font
  /// @return a tuple of actsvg digestable objects
  std::tuple<actsvg::style::fill, actsvg::style::stroke, actsvg::style::font>
  fillStrokeFont() const;
};

/// Create a group
///
/// @param objects are the individual objects to be grouped
/// @param name is the name of the group
///
/// @return a single svg object as a group
actsvg::svg::object group(const std::vector<actsvg::svg::object>& objects,
                          const std::string& name);

/// Helper method to a measure
///
/// @param xStart the start position x
/// @param yStart the start position y
/// @param xEnd the end position x
/// @param yEnd the end position y
///
/// @return a single svg object as a measure
actsvg::svg::object measure(double xStart, double yStart, double xEnd,
                            double yEnd, const std::string& variable = "",
                            double value = 0., const std::string& unit = "");

// Helper method to draw axes
///
/// @param xMin the minimum x value
/// @param xMax the maximum x value
/// @param yMin the minimum y value
/// @param yMax the maximum y value
///
/// @return an svg object
actsvg::svg::object axesXY(double xMin, double xMax, double yMin, double yMax);

// Helper method to draw axes
///
/// @param xPos the minimum x value
/// @param yPos the maximum x value
/// @param title the title of the info box
/// @param titleStyle the title of the info box
/// @param info the text of the info box
/// @param infoStyle the style of the info box (body)
/// @param object the connected object
///
/// @return an svg object
actsvg::svg::object infoBox(double xPos, double yPos, const std::string& title,
                            const Style& titleStyle,
                            const std::vector<std::string>& info,
                            const Style& infoStyle, actsvg::svg::object& object,
                            const std::vector<std::string>& highlights = {
                                "mouseover", "mouseout"});

/// Helper method to write to file
///
/// @param objects to be written out
/// @param fileName the file name is to be given
///
void toFile(const std::vector<actsvg::svg::object>& objects,
            const std::string& fileName);

}  // namespace Acts::Svg
