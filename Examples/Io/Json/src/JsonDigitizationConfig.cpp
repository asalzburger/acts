// This file is part of the Acts project.
//
// Copyright (C) 2017-2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Json/JsonDigitizationConfig.hpp"

#include "Acts/Plugins/Json/JsonHelper.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"

void to_json(nlohmann::json& j, const ActsExamples::DigitizationConfig& dc) {
  j["thickness"] = dc.thickness;
  j["segmentation"] = toJson(dc.segmentation);
  j["driftdir"] = std::array<Acts::ActsScalar, 3>{
      dc.driftDir.x(), dc.driftDir.y(), dc.driftDir.z()};
}

void from_json(const nlohmann::json& j, ActsExamples::DigitizationConfig& dc) {
  dc.thickness = j["thickness"];
  from_json(j["semgentation"], dc.segmentation);
  auto dddata = j["driftdir"];
  dc.driftDir = Acts::Vector3(dddata[0], dddata[1], dddata[2]);
}
