// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <system_error>
#include <type_traits>

namespace ActsPlugins {

/// Error codes for Geant4 stepper operations
/// @ingroup errors
enum class Geant4StepperError {
  // ensure all values are non-zero
  /// The Geant4 error propagation step failed
  PropagationFailed = 1,
  /// The Geant4 track left the Geant4 world volume
  OutsideWorld,
  /// The Geant4 track was stopped or killed during the step
  TrackKilled,
};

/// Create error code from Geant4StepperError
/// @param e The error code enum value
/// @return Standard error code
std::error_code make_error_code(Geant4StepperError e);

}  // namespace ActsPlugins

namespace std {
// register with STL
template <>
struct is_error_code_enum<ActsPlugins::Geant4StepperError> : std::true_type {};
}  // namespace std
