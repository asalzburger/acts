// This file is part of the Acts project.
//
// Copyright (C) 2016-2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Definitions.hpp"

namespace ActsFatras {

/// @brief Struct to holdf a pair of unsigned int for definition
/// of a cell, it is two dimensional for pixelated digitization,
/// but can be used for one-dimensional (e.g. strips) as well.
struct DigitizationCell {
  /// Identification and data : channel 0
  unsigned int channel0 = 0;
  /// Identification and data : channel 1
  unsigned int channel1 = 1;
  /// Identification and data : data word
  float data = 0.;
  /// Idenficiation and data: time stamp
  float time = 0.;
};

}  // namespace ActsFatras
