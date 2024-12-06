// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/SvgUtils.hpp"

Acts::Svg::Style::Style(ViewConfig vConfig) {
  fillColor = vConfig.color.rgb;
  strokeWidth = vConfig.lineThickness;
  quarterSegments = vConfig.quarterSegments;
}

std::tuple<actsvg::style::fill, actsvg::style::stroke>
Acts::Svg::Style::fillAndStroke() const {
  actsvg::style::fill fll;
  fll._fc._rgb = fillColor;
  fll._fc._opacity = fillOpacity;
  fll._fc._hl_rgb = highlightColor;
  fll._fc._highlight = highlights;

  actsvg::style::stroke str;
  str._sc._rgb = strokeColor;
  str._sc._hl_rgb = highlightStrokeColor;
  str._width = strokeWidth;
  str._hl_width = highlightStrokeWidth;
  str._dasharray = strokeDasharray;

  return {fll, str};
}

std::tuple<actsvg::style::fill, actsvg::style::stroke, actsvg::style::font>
Acts::Svg::Style::fillStrokeFont() const {
  auto [fll, str] = fillAndStroke();

  actsvg::style::font fnt;
  fnt._size = fontSize;
  fnt._fc._rgb = fontColor;

  return std::tie(fll, str, fnt);
}

actsvg::svg::object Acts::Svg::group(
    const std::vector<actsvg::svg::object>& objects, const std::string& name) {
  actsvg::svg::object gr;
  gr._tag = "g";
  gr._id = name;
  for (const auto& o : objects) {
    gr.add_object(o);
  }
  return gr;
}

actsvg::svg::object Acts::Svg::measure(double xStart, double yStart,
                                       double xEnd, double yEnd,
                                       const std::string& variable,
                                       double value, const std::string& unit) {
  std::string mlabel = "";
  if (!variable.empty()) {
    mlabel = variable + " = ";
  }
  if (value != 0.) {
    mlabel += actsvg::utils::to_string(static_cast<actsvg::scalar>(value));
  }
  if (!unit.empty()) {
    mlabel += " ";
    mlabel += unit;
  }
  return actsvg::draw::measure(
      "measure",
      {static_cast<actsvg::scalar>(xStart),
       static_cast<actsvg::scalar>(yStart)},
      {static_cast<actsvg::scalar>(xEnd), static_cast<actsvg::scalar>(yEnd)},
      actsvg::style::stroke(), actsvg::style::marker({"o"}),
      actsvg::style::marker({"|<<"}), actsvg::style::font(), mlabel);
}

actsvg::svg::object Acts::Svg::axesXY(double xMin, double xMax, double yMin,
                                      double yMax) {
  return actsvg::draw::x_y_axes(
      "x_y_axis",
      {static_cast<actsvg::scalar>(xMin), static_cast<actsvg::scalar>(xMax)},
      {static_cast<actsvg::scalar>(yMin), static_cast<actsvg::scalar>(yMax)});
}

actsvg::svg::object Acts::Svg::infoBox(
    double xPos, double yPos, const std::string& title, const Style& titleStyle,
    const std::vector<std::string>& info, const Style& infoStyle,
    actsvg::svg::object& object, const std::vector<std::string>& highlights) {
  auto [titleFill, titleStroke, titleFont] = titleStyle.fillStrokeFont();
  auto [infoFill, infoStroke, infoFont] = infoStyle.fillStrokeFont();

  actsvg::style::stroke stroke;

  return actsvg::draw::connected_info_box(
      object._id + "_infoBox",
      {static_cast<actsvg::scalar>(xPos), static_cast<actsvg::scalar>(yPos)},
      title, titleFill, titleFont, info, infoFill, infoFont, stroke, object,
      highlights);
}

void Acts::Svg::toFile(const std::vector<actsvg::svg::object>& objects,
                       const std::string& fileName) {
  actsvg::svg::file foutFile;

  for (const auto& o : objects) {
    foutFile.add_object(o);
  }

  std::ofstream fout;
  fout.open(fileName);
  fout << foutFile;
  fout.close();
}
