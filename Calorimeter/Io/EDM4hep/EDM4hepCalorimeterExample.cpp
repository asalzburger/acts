// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Calorimeter/CalorimeterDigitizationAlgorithm.hpp"
#include "ActsExamples/Calorimeter/EDM4hepCalorimeterInputConverter.hpp"
#include "ActsExamples/Calorimeter/EDM4hepCalorimeterOutputConverter.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"
#include "ActsExamples/Io/Podio/PodioReader.hpp"
#include "ActsExamples/Io/Podio/PodioWriter.hpp"
#include "ActsPlugins/EDM4hep/PodioUtil.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "ExampleFixture.hpp"

namespace {
using namespace ActsExamples;
using namespace Acts::UnitLiterals;

void checkOutput(const std::filesystem::path& path) {
  ActsPlugins::PodioUtil::ROOTReader reader;
  reader.openFile(path.string());
  if (reader.getEntries("events") != 3) {
    throw std::runtime_error("Unexpected number of calorimeter events");
  }
  for (unsigned int event = 0; event < 3; ++event) {
    podio::Frame frame = reader.readEntry("events", event);
    const auto& hits = frame.get<edm4hep::CalorimeterHitCollection>("CaloHits");
    const auto& links =
        frame.get<edm4hep::CaloHitSimCaloHitLinkCollection>("CaloLinks");
    const auto& simHits =
        frame.get<edm4hep::SimCalorimeterHitCollection>("SimCaloHits");
    if (hits.size() != 2 || links.size() != 3 || simHits.size() != 3 ||
        hits[0].getCellID() != 42 ||
        hits[1].getCellID() != CalorimeterFixture::highCellId ||
        std::abs(hits[0].getEnergy() - 0.04) > 1e-7 ||
        std::abs(hits[1].getEnergy() - 0.08) > 1e-7 ||
        std::abs(hits[1].getTime() - 4) > 1e-6 ||
        hits[1].getPosition().x != 1500) {
      throw std::runtime_error("Unexpected persistent calorimeter response");
    }
    double weightSum = 0;
    for (const auto& link : links) {
      if (!link.getFrom().isAvailable() || !link.getTo().isAvailable() ||
          link.getFrom().getCellID() != link.getTo().getCellID()) {
        throw std::runtime_error("Broken persistent calorimeter relation");
      }
      if (link.getFrom().getCellID() == CalorimeterFixture::highCellId) {
        weightSum += link.getWeight();
        const double expected =
            link.getTo().getObjectID().index == 0 ? 0.75 : 0.25;
        if (std::abs(link.getWeight() - expected) > 1e-6) {
          throw std::runtime_error("Unexpected accepted-energy link weight");
        }
      }
      for (const auto& contribution : link.getTo().getContributions()) {
        if (!contribution.isAvailable() ||
            !contribution.getParticle().isAvailable() ||
            contribution.getParticle().getPDG() != 211) {
          throw std::runtime_error("Broken contribution/particle provenance");
        }
      }
    }
    if (std::abs(weightSum - 1) > 1e-6) {
      throw std::runtime_error("Calorimeter link weights do not sum to one");
    }
  }
}
}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: ActsExampleCalorimeterEDM4hep OUTPUT_DIRECTORY\n";
    return 1;
  }
  const std::filesystem::path directory(argv[1]);
  std::filesystem::create_directories(directory);
  const auto inputPath = directory / "calorimeter-input.root";
  const auto outputPath = directory / "calorimeter-output.root";
  {
    ActsPlugins::PodioUtil::ROOTWriter writer(inputPath.string());
    for (int event = 0; event < 3; ++event) {
      auto frame = CalorimeterFixture::makeFrame();
      writer.writeFrame(frame, "events");
    }
    writer.finish();
  }
  Sequencer::Config sequence;
  sequence.events = 3;
  sequence.numThreads = 1;
  sequence.outputDir = directory.string();
  Sequencer sequencer(sequence);
  PodioReader::Config reader;
  reader.inputPath = inputPath;
  sequencer.addReader(
      std::make_shared<PodioReader>(reader, Acts::Logging::INFO));
  EDM4hepCalorimeterInputConverter::Config input;
  input.inputSimHits = "SimCaloHits";
  input.outputDeposits = "calo_deposits";
  input.outputSources = "calo_sources";
  input.cellCentre = CalorimeterFixture::cellCentre;
  sequencer.addAlgorithm(
      std::make_shared<EDM4hepCalorimeterInputConverter>(input));
  CalorimeterDigitizationAlgorithm::Config digitization;
  digitization.inputSimHits = input.outputDeposits;
  digitization.outputHits = "calo_hits";
  digitization.response.energyScale = 2;
  digitization.response.energyThreshold = 0.01_GeV;
  digitization.response.timeMin = 0_ns;
  digitization.response.timeMax = 10_ns;
  sequencer.addAlgorithm(
      std::make_shared<CalorimeterDigitizationAlgorithm>(digitization));
  EDM4hepCalorimeterOutputConverter::Config output;
  output.inputHits = digitization.outputHits;
  output.inputDeposits = input.outputDeposits;
  output.inputSources = input.outputSources;
  output.outputHits = "CaloHits";
  output.outputLinks = "CaloLinks";
  auto converter = std::make_shared<EDM4hepCalorimeterOutputConverter>(output);
  sequencer.addAlgorithm(converter);
  PodioWriter::Config writer;
  writer.inputFrame = input.inputFrame;
  writer.outputPath = outputPath.string();
  writer.category = "events";
  writer.collections = converter->collections();
  sequencer.addWriter(
      std::make_shared<PodioWriter>(writer, Acts::Logging::INFO));
  const auto result = sequencer.run();
  if (result != 0) {
    return result;
  }
  checkOutput(outputPath);
  std::cout << "Calorimeter ROOT round trip passed for three events\n";
  return 0;
}
