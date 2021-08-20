// This file is part of the Acts project.
//
// Copyright (C) 2017-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "MaterialEventAction.hpp"

#include "ActsExamples/Geant4/MaterialGeneratorAction.hpp"

#include <stdexcept>

#include <G4Event.hh>
#include <G4RunManager.hh>

#include "MaterialSteppingAction.hpp"

namespace ActsExamples::Geant4 {

MaterialEventAction* MaterialEventAction::s_instance = nullptr;

MaterialEventAction* MaterialEventAction::instance() {
  return s_instance;
}

MaterialEventAction::MaterialEventAction() : G4UserEventAction() {
  if (s_instance) {
    throw std::logic_error("Attempted to duplicate the EventAction singleton");
  } else {
    s_instance = this;
  }
}

MaterialEventAction::~MaterialEventAction() {
  s_instance = nullptr;
}

void MaterialEventAction::BeginOfEventAction(const G4Event*) {
  // reset the collection of material steps
  MaterialSteppingAction::instance()->clear();
}

void MaterialEventAction::EndOfEventAction(const G4Event* event) {
  const auto* rawPos = event->GetPrimaryVertex();
  // access the initial direction of the track
  G4ThreeVector rawDir = MaterialGeneratorAction::instance()->direction();
  // create the RecordedMaterialTrack
  Acts::RecordedMaterialTrack mtrecord;
  mtrecord.first.first =
      Acts::Vector3(rawPos->GetX0(), rawPos->GetY0(), rawPos->GetZ0());
  mtrecord.first.second = Acts::Vector3(rawDir.x(), rawDir.y(), rawDir.z());
  mtrecord.second.materialInteractions =
      MaterialSteppingAction::instance()->materialSteps();

  // write out the RecordedMaterialTrack of one event
  m_materialTracks.push_back(mtrecord);
}

/// Clear the recorded data.
void MaterialEventAction::clear() {
  m_materialTracks.clear();
}

/// Access the recorded material tracks.
///
/// This only contains valid data after the end-of-event action has been
/// executed.
const std::vector<Acts::RecordedMaterialTrack>&
MaterialEventAction::materialTracks() const {
  return m_materialTracks;
}

}  // namespace ActsExamples::Geant4