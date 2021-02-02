// This file is part of the Acts project.
//
// Copyright (C) 2019-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Plugins/Digitization/PlanarModuleStepper.hpp"
#include "ActsExamples/Digitization/DigitizationAlgorithm.hpp"
#include "ActsExamples/Digitization/DigitizationOptions.hpp"
#include "ActsExamples/Digitization/PlanarSteppingAlgorithm.hpp"
#include "ActsExamples/Digitization/SmearingAlgorithm.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"
#include "ActsExamples/Io/Csv/CsvPlanarClusterWriter.hpp"
#include "ActsExamples/Io/Json/JsonDigitizationConfig.hpp"
#include "ActsExamples/Io/Root/RootMeasurementWriter.hpp"
#include "ActsExamples/Options/CommonOptions.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <fstream>

#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#include "FatrasInternal.hpp"

void addDigitizationOptions(ActsExamples::Options::Description& desc) {
  ActsExamples::Options::addDigitizationOptions(desc);
}

void setupDigitization(
    const ActsExamples::Options::Variables& vars,
    ActsExamples::Sequencer& sequencer,
    std::shared_ptr<const ActsExamples::RandomNumbers> randomNumbers,
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry) {
  using namespace ActsExamples;

  // Read the standard options
  auto logLevel = Options::readLogLevel(vars);
  auto outputDir = vars["output-dir"].template as<std::string>();

  auto cfile = vars["digi-config-file"].as<std::string>();

  // To be handles differently between DigitizationAlgorithm/SmearingAlgorithm
  std::string clusters = "";
  std::vector<
      std::pair<Acts::GeometryIdentifier, std::vector<Acts::BoundIndices>>>
      bIndexInput;

  if (not cfile.empty()) {
    // Configuration by json file evokes 'DigitizationAlgorithm'
    auto in = std::ifstream(cfile, std::ifstream::in | std::ifstream::binary);
    if (in.good()) {
      // Get the json file for the configuration
      nlohmann::json djson;
      in >> djson;
      in.close();

      DigiConfigContainer digitizationConfigs =
          DigiConfigConverter("digitization-configuration").fromJson(djson);

      DigitizationAlgorithm::Config digiCfg =
          Options::readDigitizationConfig(vars);
      digiCfg.inputSimHits = kFatrasCollectionHits;
      digiCfg.outputMeasurements = kFatrasCollectionMeasurements;
      digiCfg.outputClusters = kFatrasCollectionClusters;
      digiCfg.outputSourceLinks = kFatrasCollectionSourceLinks;
      digiCfg.outputMeasurementParticlesMap = kFatrasMapMeasurementParticles;
      digiCfg.outputMeasurementSimHitsMap = kFatrasMapMeasurementSimHits;
      digiCfg.trackingGeometry = trackingGeometry;
      digiCfg.randomNumbers = randomNumbers;
      digiCfg.digitizationConfigs = digitizationConfigs;
      sequencer.addAlgorithm(
          std::make_shared<DigitizationAlgorithm>(digiCfg, logLevel));

      // Prepare for the (eventual) output writing
      for (size_t ibi = 0; ibi < digiCfg.digitizationConfigs.size(); ++ibi) {
        Acts::GeometryIdentifier geoID = digiCfg.digitizationConfigs.idAt(ibi);
        const auto dCfg = digiCfg.digitizationConfigs.valueAt(ibi);
        std::vector<Acts::BoundIndices> boundIndices;
        boundIndices.insert(boundIndices.end(),
                            dCfg.geometricDigiConfig.indices.begin(),
                            dCfg.geometricDigiConfig.indices.end());
        for (const auto& sConfig : dCfg.smearingDigiConfig) {
          boundIndices.push_back(sConfig.index);
        }
        bIndexInput.push_back({geoID, boundIndices});
      }
    }
  } else if (vars["digi-smear"].as<bool>()) {
    // Simpler configuration for smearing algorithm
    SmearingAlgorithm::Config smearCfg = Options::readSmearingConfig(vars);
    smearCfg.inputSimHits = kFatrasCollectionHits;
    smearCfg.outputMeasurements = kFatrasCollectionMeasurements;
    smearCfg.outputSourceLinks = kFatrasCollectionSourceLinks;
    smearCfg.outputMeasurementParticlesMap = kFatrasMapMeasurementParticles;
    smearCfg.outputMeasurementSimHitsMap = kFatrasMapMeasurementSimHits;
    smearCfg.trackingGeometry = trackingGeometry;
    smearCfg.randomNumbers = randomNumbers;
    sequencer.addAlgorithm(
        std::make_shared<SmearingAlgorithm>(smearCfg, logLevel));

    // Prepare for the (eventual) output writing
    for (size_t ibi = 0; ibi < smearCfg.smearers.size(); ++ibi) {
      Acts::GeometryIdentifier geoID = smearCfg.smearers.idAt(ibi);
      const auto sCfg = smearCfg.smearers.valueAt(ibi);
      std::vector<Acts::BoundIndices> boundIndices;

      for (const auto& sConfig : sCfg) {
        boundIndices.push_back(sConfig.index);
      }
      bIndexInput.push_back({geoID, boundIndices});
    }
  }

  // Write digitization output as ROOT files
  if (vars["output-root"].template as<bool>()) {
    RootMeasurementWriter::Config measWriterRoot;
    measWriterRoot.inputMeasurements = kFatrasCollectionMeasurements;
    measWriterRoot.inputClusters = clusters;
    measWriterRoot.inputSimHits = kFatrasCollectionHits;
    measWriterRoot.inputMeasurementSimHitsMap = kFatrasMapMeasurementSimHits;
    measWriterRoot.filePath = joinPaths(
        outputDir, std::string(kFatrasCollectionMeasurements) + ".root");
    measWriterRoot.boundIndices =
        Acts::GeometryHierarchyMap<std::vector<Acts::BoundIndices>>(
            bIndexInput);
    measWriterRoot.trackingGeometry = trackingGeometry;
    sequencer.addWriter(
        std::make_shared<RootMeasurementWriter>(measWriterRoot, logLevel));
  }
}
