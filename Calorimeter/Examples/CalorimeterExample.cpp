// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Calorimeter/CalorimeterDigitizationAlgorithm.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"

#include <cmath>
#include <memory>
#include <utility>

namespace {

using namespace Acts::UnitLiterals;

class SyntheticDeposits final : public ActsExamples::IAlgorithm {
 public:
  SyntheticDeposits()
      : IAlgorithm("SyntheticCaloDeposits",
                   Acts::getDefaultLogger("SyntheticCaloDeposits",
                                          Acts::Logging::INFO)) {
    m_output.initialize("sim_calo_hits");
  }

  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& context) const override {
    const Acts::Vector3 centre{1500_mm, 0, 0};
    // The first two deposits merge, the third is below threshold, the last
    // arrives outside the time window. No detector or shower is simulated.
    ActsCalorimeter::SimCalorimeterHitContainer deposits{
        {42, centre, 0.01_GeV, 1_ns},
        {42, centre, 0.02_GeV, 4_ns},
        {43, centre, 0.001_GeV, 2_ns},
        {44, centre, 1_GeV, 100_ns}};
    m_output(context, std::move(deposits));
    return ActsExamples::ProcessCode::SUCCESS;
  }

 private:
  ActsExamples::WriteDataHandle<ActsCalorimeter::SimCalorimeterHitContainer>
      m_output{this, "OutputSimHits"};
};

class CheckCalibratedHits final : public ActsExamples::IAlgorithm {
 public:
  CheckCalibratedHits()
      : IAlgorithm("CheckCaloHits", Acts::getDefaultLogger(
                                        "CheckCaloHits", Acts::Logging::INFO)) {
    m_input.initialize("calo_hits");
  }

  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& context) const override {
    const auto& hits = m_input(context);
    if (hits.size() != 1 || hits.front().cellId != 42 ||
        std::abs(hits.front().energy - 0.06_GeV) > 1e-12_GeV ||
        std::abs(hits.front().time - 3_ns) > 1e-12_ns ||
        hits.front().sourceIndices != std::vector<std::size_t>{0, 1}) {
      ACTS_ERROR("Unexpected prototype calorimeter response");
      return ActsExamples::ProcessCode::ABORT;
    }
    ACTS_INFO("Event " << context.eventNumber << ": cell 42, energy "
                       << hits.front().energy / 1_GeV << " GeV, time "
                       << hits.front().time / 1_ns << " ns");
    return ActsExamples::ProcessCode::SUCCESS;
  }

 private:
  ActsExamples::ReadDataHandle<ActsCalorimeter::CalorimeterHitContainer>
      m_input{this, "InputHits"};
};

}  // namespace

int main() {
  ActsExamples::Sequencer::Config sequence;
  sequence.events = 3;
  sequence.numThreads = 1;
  ActsExamples::Sequencer sequencer(sequence);
  sequencer.addAlgorithm(std::make_shared<SyntheticDeposits>());

  ActsExamples::CalorimeterDigitizationAlgorithm::Config digitization;
  digitization.inputSimHits = "sim_calo_hits";
  digitization.outputHits = "calo_hits";
  digitization.response.energyScale = 2;
  digitization.response.energyThreshold = 0.01_GeV;
  digitization.response.timeMin = 0_ns;
  digitization.response.timeMax = 10_ns;
  sequencer.addAlgorithm(
      std::make_shared<ActsExamples::CalorimeterDigitizationAlgorithm>(
          digitization));
  sequencer.addAlgorithm(std::make_shared<CheckCalibratedHits>());
  return sequencer.run();
}
