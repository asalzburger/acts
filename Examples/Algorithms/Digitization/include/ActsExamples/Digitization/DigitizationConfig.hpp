// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/BinUtility.hpp"

namespace ActsExamples {

struct DigitizationConfig {
  Acts::BinUtility segmentation;
  double thickness = 0.;
  Acts::Vector3 driftDir = Acts::Vector3(0., 0., 0.);
};
}  // namespace ActsExamples
