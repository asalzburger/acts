// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Tutorial/UserAlgorithm.hpp"

#include "Acts/Propagator/detail/SteppingLogger.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <stdexcept>

ActsExamples::UserAlgorithm::UserAlgorithm(
    ActsExamples::UserAlgorithm::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::BareAlgorithm("UserAlgorithm", lvl), m_cfg(std::move(cfg)) {
  if (m_cfg.inputStepCollection.empty()) {
    throw std::invalid_argument("Missing space point input collections");
  }
}

ActsExamples::ProcessCode ActsExamples::UserAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  using PropagationStepCollection =
      std::vector<std::vector<Acts::detail::Step>>;

  ACTS_INFO(m_cfg.message);
  auto propagationSteps =
      ctx.eventStore.get<PropagationStepCollection>(m_cfg.inputStepCollection);

  unsigned int totalSteps = 0;
  for (auto prop : propagationSteps) {
    totalSteps += prop.size();
  }

  ACTS_INFO("Successfully retrieved " << propagationSteps.size()
                                      << " propgation_step collections with "
                                      << totalSteps << " steps in total.");

  return ActsExamples::ProcessCode::SUCCESS;
}
