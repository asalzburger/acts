// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

void Acts::to_json(nlohmann::json& j, const Acts::BinningData& bd) {
  // Common to all bin utilities
  j["min"] = bd.min;
  j["max"] = bd.max;
  j["option"] =
      (bd.axisBoundaryType == Acts::AxisBoundaryType::Bound ? "open"
                                                            : "closed");
  j["value"] = bd.axisDirection;
  int bins = bd.bins();
  // Write sub bin data if present
  if (bd.subBinningData != nullptr) {
    nlohmann::json subjson;
    to_json(subjson, *bd.subBinningData);
    j["subdata"] = subjson;
    j["subadditive"] = bd.subBinningAdditive;
    // this modifies the bins as bins() returns total number in general
    if (bd.subBinningAdditive) {
      bins -= static_cast<int>(subjson["bins"]) + 1;
    } else {
      bins /= static_cast<int>(subjson["bins"]);
    }
  }
  // Now distinguish between equidistant / arbitrary
  if (bd.axisType == AxisType::Equidistant) {
    j["type"] = "equidistant";
  } else if (bd.axisType == AxisType::Variable) {
    j["type"] = "arbitrary";
    j["boundaries"] = bd.boundaries();
  }
  j["bins"] = bins;
}

void Acts::from_json(const nlohmann::json& j, BinningData& bd) {
  // Common to all bin utilities
  float min = j["min"];
  float max = j["max"];
  int bins = j["bins"];
  auto aDir = j["value"].get<AxisDirection>();
  if (bins == 1 && !(j["type"] == "arbitrary")) {
    bd = BinningData(aDir, min, max);
    return;
  }
  AxisBoundaryType bOption = (j["option"] == "open") ? AxisBoundaryType::Bound
                                                     : AxisBoundaryType::Closed;
  AxisType bType =
      (j["type"] == "equidistant") ? AxisType::Equidistant : AxisType::Variable;

  std::unique_ptr<BinningData> subBinning = nullptr;
  bool subBinningAdditive = false;
  if (j.find("subdata") != j.end()) {
    subBinningAdditive = j["subadditive"];
  }

  if (bType == AxisType::Equidistant) {
    bd = BinningData(bOption, aDir, bins, min, max, std::move(subBinning),
                     subBinningAdditive);
  } else {
    std::vector<float> boundaries = j["boundaries"];
    bd = BinningData(bOption, aDir, boundaries, std::move(subBinning));
  }
}

void Acts::to_json(nlohmann::json& j, const BinUtility& bu) {
  nlohmann::json jbindata;
  for (const auto& bdata : bu.binningData()) {
    jbindata.push_back(nlohmann::json(bdata));
  }
  j["binningdata"] = jbindata;
  if (!bu.transform().isApprox(Transform3::Identity())) {
    nlohmann::json jtrf = Transform3JsonConverter::toJson(bu.transform());
    j["transform"] = jtrf;
  }
}

void Acts::from_json(const nlohmann::json& j, Acts::BinUtility& bu) {
  bu = Acts::BinUtility();
  if (j.find("transform") != j.end() && !j["transform"].empty()) {
    Acts::Transform3 trf = Transform3JsonConverter::fromJson(j["transform"]);
    bu = Acts::BinUtility(trf);
  }
  for (const auto& jdata : j["binningdata"]) {
    Acts::BinningData bd;
    from_json(jdata, bd);
    bu += Acts::BinUtility(bd);
  }
}
