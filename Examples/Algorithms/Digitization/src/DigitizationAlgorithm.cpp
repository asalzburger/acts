// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/DigitizationAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

ActsExamples::DigitizationAlgorithm::DigitizationAlgorithm(
    ActsExamples::DigitizationAlgorithm::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::BareAlgorithm("DigitizationAlgorithm", lvl),
      m_cfg(std::move(cfg)) {
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
  }
  if (m_cfg.outputMeasurements.empty()) {
    throw std::invalid_argument("Missing measurements output collection");
  }
  if (m_cfg.outputSourceLinks.empty()) {
    throw std::invalid_argument("Missing source links output collection");
  }
  if (m_cfg.outputMeasurementParticlesMap.empty()) {
    throw std::invalid_argument(
        "Missing hit-to-particles map output collection");
  }
  if (m_cfg.outputMeasurementSimHitsMap.empty()) {
    throw std::invalid_argument(
        "Missing hit-to-simulated-hits map output collection");
  }
  if (not m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }
  if (not m_cfg.randomNumbers) {
    throw std::invalid_argument("Missing random numbers tool");
  }

  if (m_cfg.digitizationConfigs.empty()) {
    throw std::invalid_argument("Missing digitization configuration");
  }

  // create the smearers from the configuration
  std::vector<std::pair<Acts::GeometryIdentifier, Digitizer>> digitizerInput;

  for (size_t i = 0; i < m_cfg.digitizationConfigs.size(); ++i) {
    GeometricDigitizationConfig geoCfg;
    Acts::GeometryIdentifier geoId = m_cfg.digitizationConfigs.idAt(i);

    const auto& digiCfg = m_cfg.digitizationConfigs.valueAt(i);
    geoCfg = digiCfg.geometricDigiConfig;
    // Copy so we can sort in-place
    SmearingConfig smCfg = digiCfg.smearingDigiConfig;

    std::vector<Acts::BoundIndices> indices;
    for (auto& gcf : smCfg) {
      indices.push_back(gcf.index);
    }
    indices.insert(indices.begin(), geoCfg.indices.begin(),
                   geoCfg.indices.end());

    // Make sure the configured input parameter indices are sorted and unique
    std::sort(indices.begin(), indices.end());

    auto dup = std::adjacent_find(indices.begin(), indices.end());
    if (dup != indices.end()) {
      std::invalid_argument(
          "Digitization configuration contains duplicate parameter indices");
    }

    switch (smCfg.size()) {
      case 0u:
        digitizerInput.emplace_back(geoId, makeDigitizer<0u>(digiCfg));
        break;
      case 1u:
        digitizerInput.emplace_back(geoId, makeDigitizer<1u>(digiCfg));
        break;
      case 2u:
        digitizerInput.emplace_back(geoId, makeDigitizer<2u>(digiCfg));
        break;
      case 3u:
        digitizerInput.emplace_back(geoId, makeDigitizer<3u>(digiCfg));
        break;
      case 4u:
        digitizerInput.emplace_back(geoId, makeDigitizer<4u>(digiCfg));
        break;
      default:
        throw std::invalid_argument("Unsupported smearer size");
    }
  }

  m_digitizers = Acts::GeometryHierarchyMap<Digitizer>(digitizerInput);
}

ActsExamples::ProcessCode ActsExamples::DigitizationAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  // Retrieve input
  const auto& simHits = ctx.eventStore.get<SimHitContainer>(m_cfg.inputSimHits);

  // Prepare output containers
  IndexSourceLinkContainer sourceLinks;
  MeasurementContainer measurements;
  ClusterContainer clusters;
  IndexMultimap<ActsFatras::Barcode> hitParticlesMap;
  IndexMultimap<Index> hitSimHitsMap;
  sourceLinks.reserve(simHits.size());
  measurements.reserve(simHits.size());
  hitParticlesMap.reserve(simHits.size());
  hitSimHitsMap.reserve(simHits.size());

  // Setup random number generator
  auto rng = m_cfg.randomNumbers->spawnGenerator(ctx);

  ACTS_DEBUG("Starting loop over modules ...");
  for (auto simHitsGroup : groupByModule(simHits)) {
    // Manual pair unpacking instead of using
    //   auto [moduleGeoId, moduleSimHits] : ...
    // otherwise clang on macos complains that it is unable to capture the local
    // binding in the lambda used for visiting the smearer below.
    Acts::GeometryIdentifier moduleGeoId = simHitsGroup.first;
    const auto& moduleSimHits = simHitsGroup.second;

    const Acts::Surface* surfacePtr =
        m_cfg.trackingGeometry->findSurface(moduleGeoId);

    if (not surfacePtr) {
      // this is either an invalid geometry id or a misconfigured smearer
      // setup; both cases can not be handled and should be fatal.
      ACTS_ERROR("Could not find surface " << moduleGeoId
                                           << " for configured smearer");
      return ProcessCode::ABORT;
    }

    auto digitizerItr = m_digitizers.find(moduleGeoId);
    if (digitizerItr == m_digitizers.end()) {
      ACTS_DEBUG("No digitizer present for module " << moduleGeoId);
      continue;
    } else {
      ACTS_DEBUG("Digitizer found for module " << moduleGeoId);
    }

    // Run the digitizer. Iterate over the hits for this surface inside the
    // visitor so we do not need to lookup the variant object per-hit.
    std::visit(
        [&](const auto& digitizer) {
          for (auto h = moduleSimHits.begin(); h != moduleSimHits.end(); ++h) {
            const auto& simHit = *h;
            const auto simHitIdx = simHits.index_of(h);

            DigitizedParameters dParameters;

            // Geometric part - 0, 1, 2 local parameters are possible
            if (not digitizer.geometric.indices.empty()) {
              ACTS_VERBOSE("Configured to geometric digitize "
                           << digitizer.geometric.indices.size()
                           << " parameters.");
              auto channels = channelizing(digitizer.geometric, simHit,
                                           *surfacePtr, ctx.geoContext, rng);
              if (channels.empty()) {
                ACTS_DEBUG(
                    "Geometric channelization did not work, skipping this hit.")
                continue;
              }
              ACTS_VERBOSE("Activated " << channels.size()
                                        << " channels for this hit.");
              dParameters = localParameters(digitizer.geometric, channels, rng);
            }

            // Smearing part - (optionally) rest
            if (not digitizer.smearing.indices.empty()) {
              ACTS_VERBOSE("Configured to smear "
                           << digitizer.smearing.indices.size()
                           << " parameters.");
              auto res =
                  digitizer.smearing(rng, simHit, *surfacePtr, ctx.geoContext);
              if (not res.ok()) {
                ACTS_DEBUG("Problem in hit smearing, skipping this hit.")
                continue;
              }
              const auto& [par, cov] = res.value();
              for (Eigen::Index ip = 0; ip < par.rows(); ++ip) {
                dParameters.indices.push_back(digitizer.smearing.indices[ip]);
                dParameters.values.push_back(par[ip]);
                dParameters.covariances.push_back(cov(ip, ip));
              }
            }

            // Check on success - threshold could have eliminated all channels
            if (dParameters.values.empty()) {
              ACTS_VERBOSE(
                  "Parameter digitization did not yield a measurement.")
              continue;
            }

            // The measurement container is unordered and the index under which
            // the measurement will be stored is known before adding it.
            Index hitIdx = measurements.size();
            IndexSourceLink sourceLink(moduleGeoId, hitIdx);

            // Add to output containers:
            // index map and source link container are geometry-ordered.
            // since the input is also geometry-ordered, new items can
            // be added at the end.
            sourceLinks.emplace_hint(sourceLinks.end(), std::move(sourceLink));
            measurements.emplace_back(
                createMeasurement(dParameters, sourceLink));
            clusters.emplace_back(std::move(dParameters.cluster));
            // this digitization does not do hit merging so there is only one
            // mapping entry for each digitized hit.
            hitParticlesMap.emplace_hint(hitParticlesMap.end(), hitIdx,
                                         simHit.particleId());
            hitSimHitsMap.emplace_hint(hitSimHitsMap.end(), hitIdx, simHitIdx);
          }
        },
        *digitizerItr);
  }

  ctx.eventStore.add(m_cfg.outputSourceLinks, std::move(sourceLinks));
  ctx.eventStore.add(m_cfg.outputMeasurements, std::move(measurements));
  ctx.eventStore.add(m_cfg.outputClusters, std::move(clusters));
  ctx.eventStore.add(m_cfg.outputMeasurementParticlesMap,
                     std::move(hitParticlesMap));
  ctx.eventStore.add(m_cfg.outputMeasurementSimHitsMap,
                     std::move(hitSimHitsMap));
  return ProcessCode::SUCCESS;
}

std::vector<ActsFatras::Channelizer::ChannelSegment>
ActsExamples::DigitizationAlgorithm::channelizing(
    const GeometricDigitizationConfig& geoCfg, const SimHit& hit,
    const Acts::Surface& surface, const Acts::GeometryContext& gctx,
    RandomEngine& rng) const {
  Acts::Vector3 driftDir = geoCfg.drift(hit.position(), rng);

  auto driftedSegment =
      m_surfaceDrift.toReadout(gctx, surface, geoCfg.thickness, hit.position(),
                               hit.unitDirection(), driftDir);
  auto maskedSegmentRes = m_surfaceMask.apply(surface, driftedSegment);
  if (maskedSegmentRes.ok()) {
    auto maskedSegment = maskedSegmentRes.value();
    // Now Channelize
    return m_channelizer.segments(gctx, surface, geoCfg.segmentation,
                                  maskedSegment);
  }
  return {};
}

ActsExamples::DigitizationAlgorithm::DigitizedParameters
ActsExamples::DigitizationAlgorithm::localParameters(
    const GeometricDigitizationConfig& geoCfg,
    const std::vector<ActsFatras::Channelizer::ChannelSegment>& channels,
    RandomEngine& rng) const {
  DigitizedParameters dParameters;

  const auto& binningData = geoCfg.segmentation.binningData();

  Acts::ActsScalar totalWeight = 0.;
  Acts::Vector2 m(0., 0.);
  size_t b0min = SIZE_MAX;
  size_t b0max = 0;
  size_t b1min = SIZE_MAX;
  size_t b1max = 0;
  // Combine the channels
  for (const auto& ch : channels) {
    auto bin = ch.bin;
    Acts::ActsScalar charge =
        geoCfg.digital ? 1. : geoCfg.charge(ch.activation, ch.activation, rng);
    if (geoCfg.digital or charge > geoCfg.threshold) {
      totalWeight += charge;
      size_t b0 = bin[0];
      size_t b1 = bin[1];
      m += Acts::Vector2(charge * binningData[0].center(b0),
                         charge * binningData[1].center(b1));
      b0min = std::min(b0min, b0);
      b0max = std::max(b0max, b0);
      b1min = std::min(b1min, b1);
      b1max = std::max(b1max, b1);
      auto chdig = ch;
      chdig.bin = ch.bin;
      chdig.activation = charge;
      dParameters.cluster.channels.push_back(chdig);
    }
  }
  if (totalWeight > 0.) {
    m *= 1. / totalWeight;
    dParameters.indices = geoCfg.indices;
    for (auto idx : dParameters.indices) {
      dParameters.values.push_back(m[idx]);
    }
    size_t size0 = static_cast<size_t>(b0max - b0min + 1);
    size_t size1 = static_cast<size_t>(b1max - b1min + 1);
    auto covariances = geoCfg.covariance(size0, size1, rng);
    if (covariances.size() == dParameters.indices.size()) {
      dParameters.covariances = covariances;
    } else {
      dParameters.covariances =
          std::vector<Acts::ActsScalar>(dParameters.indices.size(), -1.);
    }

    dParameters.cluster.sizeLoc0 = size0;
    dParameters.cluster.sizeLoc1 = size1;
  }

  return dParameters;
}

ActsExamples::Measurement
ActsExamples::DigitizationAlgorithm::createMeasurement(
    const DigitizedParameters& dParams, const IndexSourceLink& isl) const {
  switch (dParams.indices.size()) {
    case 1u: {
      auto [indices, par, cov] = measurementConstituents<1>(dParams);
      return Acts::Measurement<IndexSourceLink, Acts::BoundIndices, 1>(
          isl, indices, par, cov);
    }
    case 2u: {
      auto [indices, par, cov] = measurementConstituents<2>(dParams);
      return Acts::Measurement<IndexSourceLink, Acts::BoundIndices, 2>(
          isl, indices, par, cov);
    };
    case 3u: {
      auto [indices, par, cov] = measurementConstituents<3>(dParams);
      return Acts::Measurement<IndexSourceLink, Acts::BoundIndices, 3>(
          isl, indices, par, cov);
    };
    case 4u: {
      auto [indices, par, cov] = measurementConstituents<4>(dParams);
      return Acts::Measurement<IndexSourceLink, Acts::BoundIndices, 4>(
          isl, indices, par, cov);
    };
  }
  std::string errorMsg = "Invalid/mismatching measurement dimension: " +
                         std::to_string(dParams.indices.size());

  throw std::runtime_error(errorMsg.c_str());
}
