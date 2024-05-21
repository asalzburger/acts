// This file is part of the Acts project.
//
// Copyright (C) 2019-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Performance/VertexPerformanceWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/MultiIndex.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Utilities/Zip.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/EventData/TruthMatching.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <stdexcept>
#include <utility>

#include <TFile.h>
#include <TTree.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;
using Acts::VectorHelpers::theta;

namespace ActsExamples {

namespace {

std::uint32_t getNumberOfReconstructableVertices(
    const SimParticleContainer& collection) {
  // map for finding frequency
  std::map<std::uint32_t, std::uint32_t> fmap;

  std::vector<std::uint32_t> reconstructableTruthVertices;

  // traverse the array for frequency
  for (const SimParticle& p : collection) {
    std::uint32_t generation = p.particleId().generation();
    if (generation > 0) {
      // truthparticle from secondary vtx
      continue;
    }
    std::uint32_t priVtxId = p.particleId().vertexPrimary();
    fmap[priVtxId]++;
  }

  // iterate over the map
  for (const auto& [priVtxId, occurrence] : fmap) {
    // Require at least 2 tracks
    if (occurrence > 1) {
      reconstructableTruthVertices.push_back(priVtxId);
    }
  }

  return reconstructableTruthVertices.size();
}

std::uint32_t getNumberOfTruePriVertices(
    const SimParticleContainer& collection) {
  // Vector to store indices of all primary vertices
  std::set<std::uint32_t> allPriVtxIds;
  for (const SimParticle& p : collection) {
    std::uint32_t priVtxId = p.particleId().vertexPrimary();
    std::uint32_t generation = p.particleId().generation();
    if (generation > 0) {
      // truthparticle from secondary vtx
      continue;
    }
    // Insert to set, removing duplicates
    allPriVtxIds.insert(priVtxId);
  }
  // Size of set corresponds to total number of primary vertices
  return allPriVtxIds.size();
}

}  // namespace

VertexPerformanceWriter::VertexPerformanceWriter(
    const VertexPerformanceWriter::Config& config, Acts::Logging::Level level)
    : WriterT(config.inputVertices, "VertexPerformanceWriter", level),
      m_cfg(config) {
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing output filename");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }
  if (m_cfg.inputTruthVertices.empty()) {
    throw std::invalid_argument("Collection with truth vertices missing");
  }
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Collection with particles missing");
  }
  if (m_cfg.inputSelectedParticles.empty()) {
    throw std::invalid_argument("Collection with selected particles missing");
  }
  if (m_cfg.inputTrackParticleMatching.empty()) {
    throw std::invalid_argument("Missing input track particles matching");
  }

  m_inputTruthVertices.initialize(m_cfg.inputTruthVertices);
  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputSelectedParticles.initialize(m_cfg.inputSelectedParticles);
  m_inputTrackParticleMatching.initialize(m_cfg.inputTrackParticleMatching);

  if (m_cfg.useTracks) {
    m_inputTracks.initialize(m_cfg.inputTracks);
  }

  // Setup ROOT I/O
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  m_outputTree->Branch("event_nr", &m_eventNr);

  m_outputTree->Branch("nRecoVtx", &m_nRecoVtx);
  m_outputTree->Branch("nTrueVtx", &m_nTrueVtx);
  m_outputTree->Branch("nVtxDetectorAcceptance", &m_nVtxDetAcceptance);
  m_outputTree->Branch("nVtxReconstructable", &m_nVtxReconstructable);

  m_outputTree->Branch("nTracksRecoVtx", &m_nTracksOnRecoVertex);

  m_outputTree->Branch("recoVertexTrackWeights", &m_recoVertexTrackWeights);

  m_outputTree->Branch("sumPt2", &m_sumPt2);

  m_outputTree->Branch("recoX", &m_recoX);
  m_outputTree->Branch("recoY", &m_recoY);
  m_outputTree->Branch("recoZ", &m_recoZ);
  m_outputTree->Branch("recoT", &m_recoT);

  m_outputTree->Branch("covXX", &m_covXX);
  m_outputTree->Branch("covYY", &m_covYY);
  m_outputTree->Branch("covZZ", &m_covZZ);
  m_outputTree->Branch("covTT", &m_covTT);
  m_outputTree->Branch("covXY", &m_covXY);
  m_outputTree->Branch("covXZ", &m_covXZ);
  m_outputTree->Branch("covXT", &m_covXT);
  m_outputTree->Branch("covYZ", &m_covYZ);
  m_outputTree->Branch("covYT", &m_covYT);
  m_outputTree->Branch("covZT", &m_covZT);

  m_outputTree->Branch("seedX", &m_seedX);
  m_outputTree->Branch("seedY", &m_seedY);
  m_outputTree->Branch("seedZ", &m_seedZ);
  m_outputTree->Branch("seedT", &m_seedT);

  m_outputTree->Branch("vertex_primary", &m_vertexPrimary);
  m_outputTree->Branch("vertex_secondary", &m_vertexSecondary);

  m_outputTree->Branch("truthVertexTrackWeights", &m_truthVertexTrackWeights);
  m_outputTree->Branch("truthVertexMatchRatio", &m_truthVertexMatchRatio);

  m_outputTree->Branch("nTracksTruthVtx", &m_nTracksOnTruthVertex);

  m_outputTree->Branch("truthX", &m_truthX);
  m_outputTree->Branch("truthY", &m_truthY);
  m_outputTree->Branch("truthZ", &m_truthZ);
  m_outputTree->Branch("truthT", &m_truthT);

  m_outputTree->Branch("resX", &m_resX);
  m_outputTree->Branch("resY", &m_resY);
  m_outputTree->Branch("resZ", &m_resZ);
  m_outputTree->Branch("resT", &m_resT);

  m_outputTree->Branch("resSeedZ", &m_resSeedZ);
  m_outputTree->Branch("resSeedT", &m_resSeedT);

  m_outputTree->Branch("pullX", &m_pullX);
  m_outputTree->Branch("pullY", &m_pullY);
  m_outputTree->Branch("pullZ", &m_pullZ);
  m_outputTree->Branch("pullT", &m_pullT);

  m_outputTree->Branch("trk_weight", &m_trkWeight);

  m_outputTree->Branch("trk_recoPhi", &m_recoPhi);
  m_outputTree->Branch("trk_recoTheta", &m_recoTheta);
  m_outputTree->Branch("trk_recoQOverP", &m_recoQOverP);

  m_outputTree->Branch("trk_recoPhiFitted", &m_recoPhiFitted);
  m_outputTree->Branch("trk_recoThetaFitted", &m_recoThetaFitted);
  m_outputTree->Branch("trk_recoQOverPFitted", &m_recoQOverPFitted);

  m_outputTree->Branch("trk_particleId", &m_trkParticleId);

  m_outputTree->Branch("trk_truthPhi", &m_truthPhi);
  m_outputTree->Branch("trk_truthTheta", &m_truthTheta);
  m_outputTree->Branch("trk_truthQOverP", &m_truthQOverP);

  m_outputTree->Branch("trk_resPhi", &m_resPhi);
  m_outputTree->Branch("trk_resTheta", &m_resTheta);
  m_outputTree->Branch("trk_resQOverP", &m_resQOverP);
  m_outputTree->Branch("trk_momOverlap", &m_momOverlap);

  m_outputTree->Branch("trk_resPhiFitted", &m_resPhiFitted);
  m_outputTree->Branch("trk_resThetaFitted", &m_resThetaFitted);
  m_outputTree->Branch("trk_resQOverPFitted", &m_resQOverPFitted);
  m_outputTree->Branch("trk_momOverlapFitted", &m_momOverlapFitted);

  m_outputTree->Branch("trk_pullPhi", &m_pullPhi);
  m_outputTree->Branch("trk_pullTheta", &m_pullTheta);
  m_outputTree->Branch("trk_pullQOverP", &m_pullQOverP);

  m_outputTree->Branch("trk_pullPhiFitted", &m_pullPhiFitted);
  m_outputTree->Branch("trk_pullThetaFitted", &m_pullThetaFitted);
  m_outputTree->Branch("trk_pullQOverPFitted", &m_pullQOverPFitted);
}

VertexPerformanceWriter::~VertexPerformanceWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ProcessCode VertexPerformanceWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  return ProcessCode::SUCCESS;
}

ProcessCode VertexPerformanceWriter::writeT(
    const AlgorithmContext& ctx, const std::vector<Acts::Vertex>& vertices) {
  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  m_nRecoVtx = vertices.size();

  ACTS_DEBUG("Number of reco vertices in event: " << m_nRecoVtx);

  // Read truth vertex input collection
  const SimVertexContainer& truthVertices = m_inputTruthVertices(ctx);
  // Read truth particle input collection
  const SimParticleContainer& particles = m_inputParticles(ctx);
  const SimParticleContainer& selectedParticles = m_inputSelectedParticles(ctx);
  const TrackParticleMatching& trackParticleMatching =
      m_inputTrackParticleMatching(ctx);

  // Get number of generated true primary vertices
  m_nTrueVtx = getNumberOfTruePriVertices(particles);
  // Get number of detector-accepted true primary vertices
  m_nVtxDetAcceptance = getNumberOfTruePriVertices(selectedParticles);

  ACTS_DEBUG("Number of truth particles in event : " << particles.size());
  ACTS_DEBUG("Number of truth primary vertices : " << m_nTrueVtx);
  ACTS_DEBUG("Number of detector-accepted truth primary vertices : "
             << m_nVtxDetAcceptance);

  const ConstTrackContainer* tracks = nullptr;
  SimParticleContainer recoParticles;

  // Get the event number
  m_eventNr = ctx.eventNumber;

  auto findParticle = [&](ConstTrackProxy track) -> std::optional<SimParticle> {
    // Get the truth-matched particle
    auto imatched = trackParticleMatching.find(track.index());
    if (imatched == trackParticleMatching.end() ||
        !imatched->second.particle.has_value()) {
      ACTS_DEBUG("No truth particle associated with this track, index = "
                 << track.index() << " tip index = " << track.tipIndex());
      return {};
    }

    const TrackMatchEntry& particleMatch = imatched->second;

    auto iparticle = particles.find(particleMatch.particle->value());
    if (iparticle == particles.end()) {
      ACTS_DEBUG(
          "Truth particle found but not monitored with this track, index = "
          << track.index() << " tip index = " << track.tipIndex()
          << " and this barcode = " << particleMatch.particle->value());
      return {};
    }

    return *iparticle;
  };

  auto weightHighEnough = [this](const Acts::TrackAtVertex& trkAtVtx) {
    return trkAtVtx.trackWeight > m_cfg.minTrkWeight;
  };

  // Helper function for computing the pull
  auto pull =
      [this](const Acts::ActsScalar& diff, const Acts::ActsScalar& variance,
             const std::string& variableStr, const bool& afterFit = true) {
        if (variance <= 0) {
          std::string tempStr;
          if (afterFit) {
            tempStr = "after";
          } else {
            tempStr = "before";
          }
          ACTS_WARNING("Nonpositive variance "
                       << tempStr << " vertex fit: Var(" << variableStr
                       << ") = " << variance << " <= 0.");
          return std::numeric_limits<double>::quiet_NaN();
        }
        double std = std::sqrt(variance);
        return diff / std;
      };

  if (m_cfg.useTracks) {
    tracks = &m_inputTracks(ctx);

    for (ConstTrackProxy track : *tracks) {
      if (!track.hasReferenceSurface()) {
        ACTS_DEBUG("No reference surface on this track, index = "
                   << track.index() << " tip index = " << track.tipIndex());
        continue;
      }

      if (std::optional<SimParticle> particle = findParticle(track);
          particle.has_value()) {
        recoParticles.insert(*particle);
      }
    }
  } else {
    // if not using tracks, then all truth particles are associated with the
    // vertex
    recoParticles = particles;
  }

  // Get number of track-associated true primary vertices
  m_nVtxReconstructable = getNumberOfReconstructableVertices(recoParticles);

  ACTS_INFO("Number of reconstructed tracks : "
            << ((tracks != nullptr) ? tracks->size() : 0));
  ACTS_INFO("Number of reco track-associated truth particles in event : "
            << recoParticles.size());
  ACTS_INFO("Maximum number of reconstructible primary vertices : "
            << m_nVtxReconstructable);

  // Loop over reconstructed vertices and see if they can be matched to a true
  // vertex.
  for (const Acts::Vertex& vtx : vertices) {
    // Reconstructed tracks that contribute to the reconstructed vertex
    const std::vector<Acts::TrackAtVertex>& tracksAtVtx = vtx.tracks();
    // Input tracks matched to `tracksAtVtx`
    std::vector<std::uint32_t> trackIndices;

    // Containers for storing truth particles and truth vertices that contribute
    // to the reconstructed vertex
    SimParticleContainer particlesAtVtx;
    std::vector<std::pair<SimBarcode, double>> contributingTruthVertices;

    if (m_cfg.useTracks) {
      for (const Acts::TrackAtVertex& trk : tracksAtVtx) {
        // Track parameters before the vertex fit
        const Acts::BoundTrackParameters& origTrack =
            *trk.originalParams.as<Acts::BoundTrackParameters>();

        bool foundMatchingParticle = false;

        // Finding the matching parameters in the container of all track
        // parameters. This allows us to identify the corresponding particle.
        // TODO this should not be necessary if the tracks at vertex would keep
        // this information
        for (ConstTrackProxy inputTrk : *tracks) {
          const auto& params = inputTrk.parameters();

          if (origTrack.parameters() == params) {
            trackIndices.push_back(inputTrk.index());
            foundMatchingParticle = true;

            if (trk.trackWeight > m_cfg.minTrkWeight) {
              std::optional<SimParticle> particleOpt = findParticle(inputTrk);
              if (!particleOpt.has_value()) {
                continue;
              }
              const SimParticle& particle = *particleOpt;

              particlesAtVtx.insert(particle);
              contributingTruthVertices.emplace_back(
                  particle.particleId().vertexId(), trk.trackWeight);
            }

            break;
          }
        }

        if (!foundMatchingParticle) {
          ACTS_DEBUG("Track has no matching truth particle.");
        }
      }

      if (tracksAtVtx.size() != trackIndices.size()) {
        ACTS_ERROR("Not all tracks at vertex have a matching input track.");
      }
    } else {
      for (const SimParticle& particle : particles) {
        contributingTruthVertices.emplace_back(particle.particleId().vertexId(),
                                               1);
      }
    }

    double recoVertexTrackWeights = 0;
    for (const Acts::TrackAtVertex& trk : tracksAtVtx) {
      recoVertexTrackWeights += trk.trackWeight;
    }

    // Find true vertex that contributes most to the reconstructed vertex
    std::map<SimVertexBarcode, double> truthVertexWeights;
    for (const auto& [vtxId, weight] : contributingTruthVertices) {
      truthVertexWeights[vtxId] += weight;
    }
    auto truthVertexMatch = std::max_element(
        truthVertexWeights.begin(), truthVertexWeights.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    if (truthVertexMatch == truthVertexWeights.end()) {
      ACTS_DEBUG("No truth vertex found for reconstructed vertex.");
      continue;
    }
    SimVertexBarcode truthVertexId = truthVertexMatch->first;
    double truthVertexTrackWeights = truthVertexMatch->second;

    // Count number of reconstructible tracks on truth vertex
    std::uint32_t nTracksOnTruthVertex = 0u;
    for (const SimParticle& particle : selectedParticles) {
      SimBarcode vtxId =
          SimBarcode(particle.particleId()).setParticle(0).setSubParticle(0);
      if (vtxId == truthVertexId) {
        ++nTracksOnTruthVertex;
      }
    }

    // Get number of contributing tracks (i.e., tracks with a weight above
    // threshold)
    std::uint32_t nTracksOnRecoVertex =
        std::count_if(tracksAtVtx.begin(), tracksAtVtx.end(), weightHighEnough);
    // Match reconstructed and truth vertex if the tracks of the truth vertex
    // make up at least vertexMatchThreshold of the track weight at the
    // reconstructed vertex.
    double vertexMatchFraction =
        (m_cfg.useTracks ? truthVertexTrackWeights / recoVertexTrackWeights
                         : 1.0);

    if (vertexMatchFraction <= m_cfg.vertexMatchThreshold) {
      ACTS_DEBUG("Match fraction " << vertexMatchFraction
                                   << " below threshold. Not matching.");
      continue;
    }

    auto iTruthVertex = truthVertices.find(truthVertexId);
    if (iTruthVertex == truthVertices.end()) {
      ACTS_ERROR("Truth vertex not found.");
      continue;
    }
    const SimVertex& truthVertex = *iTruthVertex;

    const Acts::ActsVector<4>& truePos = truthVertex.position4;

    // Write vertex truth based information
    {
      m_nTracksOnRecoVertex.push_back(nTracksOnRecoVertex);
      m_recoVertexTrackWeights.push_back(recoVertexTrackWeights);

      m_recoX.push_back(vtx.fullPosition()[Acts::FreeIndices::eFreePos0]);
      m_recoY.push_back(vtx.fullPosition()[Acts::FreeIndices::eFreePos1]);
      m_recoZ.push_back(vtx.fullPosition()[Acts::FreeIndices::eFreePos2]);
      m_recoT.push_back(vtx.fullPosition()[Acts::FreeIndices::eFreeTime]);

      Acts::ActsScalar varX = vtx.fullCovariance()(
          Acts::FreeIndices::eFreePos0, Acts::FreeIndices::eFreePos0);
      Acts::ActsScalar varY = vtx.fullCovariance()(
          Acts::FreeIndices::eFreePos1, Acts::FreeIndices::eFreePos1);
      Acts::ActsScalar varZ = vtx.fullCovariance()(
          Acts::FreeIndices::eFreePos2, Acts::FreeIndices::eFreePos2);
      Acts::ActsScalar varTime = vtx.fullCovariance()(
          Acts::FreeIndices::eFreeTime, Acts::FreeIndices::eFreeTime);
      m_covXX.push_back(varX);
      m_covYY.push_back(varY);
      m_covZZ.push_back(varZ);
      m_covTT.push_back(varTime);
      m_covXY.push_back(vtx.fullCovariance()(Acts::FreeIndices::eFreePos0,
                                             Acts::FreeIndices::eFreePos1));
      m_covXZ.push_back(vtx.fullCovariance()(Acts::FreeIndices::eFreePos0,
                                             Acts::FreeIndices::eFreePos2));
      m_covXT.push_back(vtx.fullCovariance()(Acts::FreeIndices::eFreePos0,
                                             Acts::FreeIndices::eFreeTime));
      m_covYZ.push_back(vtx.fullCovariance()(Acts::FreeIndices::eFreePos1,
                                             Acts::FreeIndices::eFreePos2));
      m_covYT.push_back(vtx.fullCovariance()(Acts::FreeIndices::eFreePos1,
                                             Acts::FreeIndices::eFreeTime));
      m_covZT.push_back(vtx.fullCovariance()(Acts::FreeIndices::eFreePos2,
                                             Acts::FreeIndices::eFreeTime));

      m_seedX.push_back(vtx.fullSeedPosition()[Acts::FreeIndices::eFreePos0]);
      m_seedY.push_back(vtx.fullSeedPosition()[Acts::FreeIndices::eFreePos1]);
      m_seedZ.push_back(vtx.fullSeedPosition()[Acts::FreeIndices::eFreePos2]);
      m_seedT.push_back(vtx.fullSeedPosition()[Acts::FreeIndices::eFreeTime]);

      m_vertexPrimary.push_back(truthVertex.vertexId().vertexPrimary());
      m_vertexSecondary.push_back(truthVertex.vertexId().vertexSecondary());

      m_truthVertexTrackWeights.push_back(truthVertexTrackWeights);
      m_truthVertexMatchRatio.push_back(vertexMatchFraction);

      m_nTracksOnTruthVertex.push_back(nTracksOnTruthVertex);

      m_truthX.push_back(truePos[Acts::FreeIndices::eFreePos0]);
      m_truthY.push_back(truePos[Acts::FreeIndices::eFreePos1]);
      m_truthZ.push_back(truePos[Acts::FreeIndices::eFreePos2]);
      m_truthT.push_back(truePos[Acts::FreeIndices::eFreeTime]);

      const Acts::ActsVector<4> diffPos = vtx.fullPosition() - truePos;
      m_resX.push_back(diffPos[Acts::FreeIndices::eFreePos0]);
      m_resY.push_back(diffPos[Acts::FreeIndices::eFreePos1]);
      m_resZ.push_back(diffPos[Acts::FreeIndices::eFreePos2]);
      m_resT.push_back(diffPos[Acts::FreeIndices::eFreeTime]);

      const Acts::ActsVector<4> diffSeedPos = vtx.fullSeedPosition() - truePos;
      m_resSeedZ.push_back(diffSeedPos[Acts::FreeIndices::eFreePos2]);
      m_resSeedT.push_back(diffSeedPos[Acts::FreeIndices::eFreeTime]);

      m_pullX.push_back(pull(diffPos[Acts::FreeIndices::eFreePos0], varX, "X"));
      m_pullY.push_back(pull(diffPos[Acts::FreeIndices::eFreePos1], varY, "Y"));
      m_pullZ.push_back(pull(diffPos[Acts::FreeIndices::eFreePos2], varZ, "Z"));
      m_pullT.push_back(
          pull(diffPos[Acts::FreeIndices::eFreeTime], varTime, "T"));

      double sumPt2 = 0.;
      for (const Acts::TrackAtVertex& trk : tracksAtVtx) {
        if (trk.trackWeight > m_cfg.minTrkWeight) {
          double pt = trk.originalParams.as<Acts::BoundTrackParameters>()
                          ->transverseMomentum();
          sumPt2 += pt * pt;
        }
      }
      m_sumPt2.push_back(sumPt2);
    }

    // Write vertex track based information
    {
      // Get references to inner vectors where all track variables corresponding
      // to the current vertex will be saved

      auto& innerTrkWeight = m_trkWeight.emplace_back();

      auto& innerRecoPhi = m_recoPhi.emplace_back();
      auto& innerRecoTheta = m_recoTheta.emplace_back();
      auto& innerRecoQOverP = m_recoQOverP.emplace_back();

      auto& innerRecoPhiFitted = m_recoPhiFitted.emplace_back();
      auto& innerRecoThetaFitted = m_recoThetaFitted.emplace_back();
      auto& innerRecoQOverPFitted = m_recoQOverPFitted.emplace_back();

      auto& innerTrkParticleId = m_trkParticleId.emplace_back();

      auto& innerTruthPhi = m_truthPhi.emplace_back();
      auto& innerTruthTheta = m_truthTheta.emplace_back();
      auto& innerTruthQOverP = m_truthQOverP.emplace_back();

      auto& innerResPhi = m_resPhi.emplace_back();
      auto& innerResTheta = m_resTheta.emplace_back();
      auto& innerResQOverP = m_resQOverP.emplace_back();

      auto& innerResPhiFitted = m_resPhiFitted.emplace_back();
      auto& innerResThetaFitted = m_resThetaFitted.emplace_back();
      auto& innerResQOverPFitted = m_resQOverPFitted.emplace_back();

      auto& innerMomOverlap = m_momOverlap.emplace_back();
      auto& innerMomOverlapFitted = m_momOverlapFitted.emplace_back();

      auto& innerPullPhi = m_pullPhi.emplace_back();
      auto& innerPullTheta = m_pullTheta.emplace_back();
      auto& innerPullQOverP = m_pullQOverP.emplace_back();

      auto& innerPullPhiFitted = m_pullPhiFitted.emplace_back();
      auto& innerPullThetaFitted = m_pullThetaFitted.emplace_back();
      auto& innerPullQOverPFitted = m_pullQOverPFitted.emplace_back();

      // We compare the reconstructed momenta to the true momenta at the
      // vertex. For this, we propagate the reconstructed tracks to the PCA of
      // the true vertex position. Setting up propagator:
      Acts::EigenStepper<> stepper(m_cfg.bField);
      using Propagator = Acts::Propagator<Acts::EigenStepper<>>;
      auto propagator = std::make_shared<Propagator>(stepper);
      // Setting the geometry/magnetic field context for the event
      Acts::PropagatorOptions pOptions(ctx.geoContext, ctx.magFieldContext);

      // Saving the reconstructed/truth momenta. The reconstructed momenta
      // are taken at the PCA to the truth vertex position -> we need to
      // perform a propagation.

      // Perigee at the true vertex position
      const std::shared_ptr<Acts::PerigeeSurface> perigeeSurface =
          Acts::Surface::makeShared<Acts::PerigeeSurface>(truePos.head(3));
      // Lambda for propagating the tracks to the PCA
      auto propagateToVtx = [&](const Acts::BoundTrackParameters& params)
          -> std::optional<Acts::BoundTrackParameters> {
        Acts::SurfaceIntersection intersection =
            perigeeSurface
                ->intersect(ctx.geoContext, params.position(ctx.geoContext),
                            params.direction(), Acts::BoundaryCheck(false))
                .closest();
        pOptions.direction = Acts::Direction::fromScalarZeroAsPositive(
            intersection.pathLength());

        auto result = propagator->propagate(params, *perigeeSurface, pOptions);
        if (!result.ok()) {
          ACTS_ERROR("Propagation to true vertex position failed.");
          return std::nullopt;
        }
        auto& paramsAtVtx = *result->endParameters;
        return std::make_optional(paramsAtVtx);
      };

      for (const auto& [trkAtVtx, trkIndex] :
           Acts::zip(tracksAtVtx, trackIndices)) {
        const auto trk = tracks->getTrack(trkIndex);

        innerTrkWeight.push_back(trkAtVtx.trackWeight);

        std::optional<SimParticle> particleOpt = findParticle(trk);
        if (!particleOpt.has_value()) {
          continue;
        }
        const SimParticle& particle = *particleOpt;
        innerTrkParticleId.push_back(particle.particleId().value());

        const Acts::Vector3& trueUnitDir = particle.direction();
        Acts::Vector3 trueMom;
        trueMom.head(2) = Acts::makePhiThetaFromDirection(trueUnitDir);
        trueMom[2] = particle.qOverP();
        innerTruthPhi.push_back(trueMom[0]);
        innerTruthTheta.push_back(trueMom[1]);
        innerTruthQOverP.push_back(trueMom[2]);

        // Save track parameters before the vertex fit
        const auto paramsAtVtx = propagateToVtx(
            *trkAtVtx.originalParams.as<Acts::BoundTrackParameters>());
        if (paramsAtVtx != std::nullopt) {
          Acts::ActsVector<3> recoMom =
              paramsAtVtx->parameters().segment(Acts::eBoundPhi, 3);
          const Acts::ActsMatrix<3, 3>& momCov =
              paramsAtVtx->covariance()->block<3, 3>(Acts::eBoundPhi,
                                                     Acts::eBoundPhi);
          innerRecoPhi.push_back(recoMom[0]);
          innerRecoTheta.push_back(recoMom[1]);
          innerRecoQOverP.push_back(recoMom[2]);

          Acts::ActsVector<3> diffMom = recoMom - trueMom;
          // Accounting for the periodicity of phi. We overwrite the
          // previously computed value for better readability.
          diffMom[0] = Acts::detail::difference_periodic(recoMom(0), trueMom(0),
                                                         2 * M_PI);
          innerResPhi.push_back(diffMom[0]);
          innerResTheta.push_back(diffMom[1]);
          innerResQOverP.push_back(diffMom[2]);

          innerPullPhi.push_back(pull(diffMom[0], momCov(0, 0), "phi", false));
          innerPullTheta.push_back(
              pull(diffMom[1], momCov(1, 1), "theta", false));
          innerPullQOverP.push_back(
              pull(diffMom[2], momCov(2, 2), "q/p", false));

          const Acts::Vector3& recoUnitDir = paramsAtVtx->direction();
          double overlap = trueUnitDir.dot(recoUnitDir);
          innerMomOverlap.push_back(overlap);
        }

        // Save track parameters after the vertex fit
        const auto paramsAtVtxFitted = propagateToVtx(trkAtVtx.fittedParams);
        if (paramsAtVtxFitted != std::nullopt &&
            trkAtVtx.trackWeight > m_cfg.minTrkWeight) {
          Acts::ActsVector<3> recoMomFitted =
              paramsAtVtxFitted->parameters().segment(Acts::eBoundPhi, 3);
          const Acts::ActsMatrix<3, 3>& momCovFitted =
              paramsAtVtxFitted->covariance()->block<3, 3>(Acts::eBoundPhi,
                                                           Acts::eBoundPhi);
          innerRecoPhiFitted.push_back(recoMomFitted[0]);
          innerRecoThetaFitted.push_back(recoMomFitted[1]);
          innerRecoQOverPFitted.push_back(recoMomFitted[2]);

          Acts::ActsVector<3> diffMomFitted = recoMomFitted - trueMom;
          // Accounting for the periodicity of phi. We overwrite the
          // previously computed value for better readability.
          diffMomFitted[0] = Acts::detail::difference_periodic(
              recoMomFitted(0), trueMom(0), 2 * M_PI);
          innerResPhiFitted.push_back(diffMomFitted[0]);
          innerResThetaFitted.push_back(diffMomFitted[1]);
          innerResQOverPFitted.push_back(diffMomFitted[2]);

          innerPullPhiFitted.push_back(
              pull(diffMomFitted[0], momCovFitted(0, 0), "phi"));
          innerPullThetaFitted.push_back(
              pull(diffMomFitted[1], momCovFitted(1, 1), "theta"));
          innerPullQOverPFitted.push_back(
              pull(diffMomFitted[2], momCovFitted(2, 2), "q/p"));

          const Acts::Vector3& recoUnitDirFitted =
              paramsAtVtxFitted->direction();
          double overlapFitted = trueUnitDir.dot(recoUnitDirFitted);
          innerMomOverlapFitted.push_back(overlapFitted);
        }
      }
    }
  }

  // fill the variables
  m_outputTree->Fill();

  m_nTracksOnRecoVertex.clear();
  m_recoVertexTrackWeights.clear();
  m_recoX.clear();
  m_recoY.clear();
  m_recoZ.clear();
  m_recoT.clear();
  m_covXX.clear();
  m_covYY.clear();
  m_covZZ.clear();
  m_covTT.clear();
  m_covXY.clear();
  m_covXZ.clear();
  m_covXT.clear();
  m_covYZ.clear();
  m_covYT.clear();
  m_covZT.clear();
  m_seedX.clear();
  m_seedY.clear();
  m_seedZ.clear();
  m_seedT.clear();
  m_vertexPrimary.clear();
  m_vertexSecondary.clear();
  m_truthVertexTrackWeights.clear();
  m_truthVertexMatchRatio.clear();
  m_nTracksOnTruthVertex.clear();
  m_truthX.clear();
  m_truthY.clear();
  m_truthZ.clear();
  m_truthT.clear();
  m_resX.clear();
  m_resY.clear();
  m_resZ.clear();
  m_resT.clear();
  m_resSeedZ.clear();
  m_resSeedT.clear();
  m_pullX.clear();
  m_pullY.clear();
  m_pullZ.clear();
  m_pullT.clear();
  m_sumPt2.clear();
  m_trkWeight.clear();
  m_recoPhi.clear();
  m_recoTheta.clear();
  m_recoQOverP.clear();
  m_recoPhiFitted.clear();
  m_recoThetaFitted.clear();
  m_recoQOverPFitted.clear();
  m_trkParticleId.clear();
  m_truthPhi.clear();
  m_truthTheta.clear();
  m_truthQOverP.clear();
  m_resPhi.clear();
  m_resTheta.clear();
  m_resQOverP.clear();
  m_momOverlap.clear();
  m_resPhiFitted.clear();
  m_resThetaFitted.clear();
  m_resQOverPFitted.clear();
  m_momOverlapFitted.clear();
  m_pullPhi.clear();
  m_pullTheta.clear();
  m_pullQOverP.clear();
  m_pullPhiFitted.clear();
  m_pullThetaFitted.clear();
  m_pullQOverPFitted.clear();

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
