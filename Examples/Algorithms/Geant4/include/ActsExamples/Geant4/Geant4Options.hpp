// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Utilities/OptionsFwd.hpp"

#include "Geant4MaterialRecording.hpp"

namespace ActsExamples {

namespace Options {

/// @brief Geant4 specific options
///
/// @param desc The option descrion forward
void addGeant4Options(Description& desc);

/// Read the Geatn4 options and @return a Geant4MaterialRecording::Config
///
/// @tparam vmap_t is the Type of the Parameter map to be read out
///
/// @param variables is the parameter map for the options
///
/// @returns a Config object for the Geant4MaterialRecording
Geant4MaterialRecording::Config readGeant4MaterialRecordingConfig(
    const Variables& variables);

}  // namespace Options
}  // namespace ActsExamples
