// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Nodd/DD4hepNoddDetector.hpp"

namespace ActsExamples {

DD4hepNoddDetector::Config::Config() {
  name = "NoddPixelDetector";
}

DD4hepNoddDetector::DD4hepNoddDetector(const Config& cfg)
    : DD4hepDetectorBase{cfg}, m_cfg{cfg} {}

auto DD4hepNoddDetector::config() const -> const Config& {
  return m_cfg;
}

}  // namespace ActsExamples
