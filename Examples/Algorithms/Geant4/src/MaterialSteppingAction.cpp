// This file is part of the Acts project.
//
// Copyright (C) 2017-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "MaterialSteppingAction.hpp"

#include "Acts/Definitions/Units.hpp"

#include <stdexcept>

#include <G4Material.hh>
#include <G4Step.hh>

namespace ActsExamples::Geant4 {

MaterialSteppingAction* MaterialSteppingAction::s_instance = nullptr;

MaterialSteppingAction* MaterialSteppingAction::instance() {
  return s_instance;
}

MaterialSteppingAction::MaterialSteppingAction() : G4UserSteppingAction() {
  if (s_instance) {
    throw std::logic_error(
        "Attempted to duplicate the SteppingAction singleton");
  } else {
    s_instance = this;
  }
}

MaterialSteppingAction::~MaterialSteppingAction() {
  s_instance = nullptr;
}

void MaterialSteppingAction::UserSteppingAction(const G4Step* step) {
  // get the material
  G4Material* material = step->GetPreStepPoint()->GetMaterial();

  if (material && material->GetName() != "Vacuum" &&
      material->GetName() != "Air") {
    // Quantities valid for elemental materials and mixtures
    double X0 = (material->GetRadlen() / CLHEP::mm) * Acts::UnitConstants::mm;
    double L0 = (material->GetNuclearInterLength() / CLHEP::mm) *
                Acts::UnitConstants::mm;
    double rho = (material->GetDensity() / (CLHEP::gram / CLHEP::mm3)) *
                 (Acts::UnitConstants::g / Acts::UnitConstants::mm3);

    // Get{A,Z} is only meaningful for single-element materials (according to
    // the Geant4 docs). Need to compute average manually.
    const G4ElementVector* elements = material->GetElementVector();
    const G4double* fraction = material->GetFractionVector();
    size_t nElements = material->GetNumberOfElements();
    double Ar = 0.;
    double Z = 0.;
    if (nElements == 1) {
      Ar = material->GetA() / (CLHEP::gram / CLHEP::mole);
      Z = material->GetZ();
    } else {
      for (size_t i = 0; i < nElements; i++) {
        Ar +=
            elements->at(i)->GetA() * fraction[i] / (CLHEP::gram / CLHEP::mole);
        Z += elements->at(i)->GetZ() * fraction[i];
      }
    }
    // construct passed material slab for the step
    const auto slab = Acts::MaterialSlab(
        Acts::Material::fromMassDensity(X0, L0, Ar, Z, rho),
        (step->GetStepLength() / CLHEP::mm) * Acts::UnitConstants::mm);

    // create the RecordedMaterialSlab
    const auto& rawPos = step->GetPreStepPoint()->GetPosition();
    const auto& rawDir = step->GetPreStepPoint()->GetMomentum();
    Acts::MaterialInteraction mInteraction;
    mInteraction.position = Acts::Vector3(rawPos.x(), rawPos.y(), rawPos.z());
    mInteraction.direction = Acts::Vector3(rawDir.x(), rawDir.y(), rawDir.z());
    mInteraction.direction.normalized();
    mInteraction.materialSlab = slab;
    mInteraction.pathCorrection = (step->GetStepLength() / CLHEP::mm);
    m_materialSteps.push_back(mInteraction);
  }
}

void MaterialSteppingAction::clear() {
  m_materialSteps.clear();
  m_trackSteps.clear();
}

}  // namespace ActsExamples::Geant4