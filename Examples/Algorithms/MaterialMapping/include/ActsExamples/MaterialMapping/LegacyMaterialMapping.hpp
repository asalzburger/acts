// This file is part of the Acts project.
//
// Copyright (C) 2017-2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/DetectorMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/LegacySurfaceMaterialMapper.hpp"
#include "Acts/Material/LegacyVolumeMaterialMapper.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ActsExamples {
class IMaterialWriter;
struct AlgorithmContext;
}  // namespace ActsExamples

namespace Acts {

class TrackingGeometry;

}  // namespace Acts

namespace ActsExamples {

/// @class LegacyMaterialMapping
///
/// @brief Initiates and executes material mapping
///
/// The LegacyMaterialMapping reads in the MaterialTrack with a dedicated
/// reader and uses the material mapper to project the material onto
/// the tracking geometry
///
/// By construction, the material mapping needs inter-event information
/// to build the material maps of accumulated single particle views.
/// However, running it in one single event, puts enormous pressure onto
/// the I/O structure.
///
/// It therefore saves the mapping state/cache as a private member variable
/// and is designed to be executed in a single threaded mode.
class LegacyMaterialMapping : public IAlgorithm {
 public:
  /// @class nested Config class
  /// of the LegacyLegacyMaterialMapping algorithm
  struct Config {
    /// Input collection
    std::string collection = "material_tracks";

    /// The material collection to be stored
    std::string mappingMaterialCollection = "mapped_material_tracks";

    /// The ACTS surface material mapper
    std::shared_ptr<Acts::LegacySurfaceMaterialMapper> materialSurfaceMapper =
        nullptr;

    /// The ACTS volume material mapper
    std::shared_ptr<Acts::LegacyVolumeMaterialMapper> materialVolumeMapper = nullptr;

    /// The writer of the material
    std::vector<std::shared_ptr<IMaterialWriter>> materialWriters{};

    /// The TrackingGeometry to be mapped on
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry = nullptr;
  };

  /// Constructor
  ///
  /// @param cfg The configuration struct carrying the used tools
  /// @param level The output logging level
  LegacyMaterialMapping(const Config& cfg,
                  Acts::Logging::Level level = Acts::Logging::INFO);

  /// Destructor
  /// - it also writes out the file
  ~LegacyMaterialMapping() override;

  /// Framework execute method
  ///
  /// @param context The algorithm context for event consistency
  ActsExamples::ProcessCode execute(
      const AlgorithmContext& context) const override;

  /// Return the parameters to optimised the material map for a given surface
  /// Those parameters are the variance and the number of track for each bin
  ///
  /// @param surfaceID the ID of the surface of interest
  std::vector<std::pair<double, int>> scoringParameters(uint64_t surfaceID);

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;  //!< internal config object
  std::unique_ptr<Acts::IMaterialMapper::State> m_mappingState =
      nullptr;  //!< Material mapping state
  std::unique_ptr<Acts::IMaterialMapper::State> m_mappingStateVol =
      nullptr;  //!< Material mapping state

  ReadDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_inputMaterialTracks{this, "InputMaterialTracks"};
  WriteDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_outputMaterialTracks{this, "OutputMaterialTracks"};
};

}  // namespace ActsExamples
