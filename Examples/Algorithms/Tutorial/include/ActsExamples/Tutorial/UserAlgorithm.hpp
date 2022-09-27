// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/BareAlgorithm.hpp"

#include <string>
#include <vector>

namespace ActsExamples {

/// Construct a user algorithm for demonstrator purposes
class UserAlgorithm final : public BareAlgorithm {
 public:
  struct Config {
    /// Simple message
    std::string message = "Hello world";
  };

  /// Construct the user algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  UserAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Run the seeding algorithm.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final override;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
