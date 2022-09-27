// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Tutorial/UserAlgorithm.hpp"

#include <stdexcept>

ActsExamples::UserAlgorithm::UserAlgorithm(
    ActsExamples::UserAlgorithm::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::BareAlgorithm("UserAlgorithm", lvl),
      m_cfg(std::move(cfg)) {

}

ActsExamples::ProcessCode ActsExamples::UserAlgorithm::execute(
    const AlgorithmContext& ctx) const {

  ACTS_INFO(m_cfg.message);
  
  return ActsExamples::ProcessCode::SUCCESS;
}
