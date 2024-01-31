// This file is part of the Acts project.
//
// Copyright (C) 2017-2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/DetectorMaterial.hpp"
#include "Acts/Material/interface/IMaterialMapper.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ActsExamples {
class IMaterialWriter;
struct AlgorithmContext;
}  // namespace ActsExamples

namespace ActsExamples {

/// @class MaterialMapping
///
/// @brief Initiates and executes material mapping in its most simple
/// form. It takes a single material mapper, reads the material tracks,
/// maps and writes mapped, unmapped tracks such as final maps.
///
/// By construction, the material mapping needs inter-event information
/// to build the material maps of accumulated single particle views.
/// However, running it in one single event, puts enormous pressure onto
/// the I/O structure.
///
/// It therefore saves the mapping state/cache as a private member variable
/// and is designed to be executed in a single threaded mode.
class MaterialMapping : public IAlgorithm {
 public:
  /// @class nested Config class
  /// of the MaterialMapping algorithm
  struct Config {
    using Mapper = std::shared_ptr<const Acts::IMaterialMapper>;

    /// Input collection: input material maps
    std::string collection = "material_tracks";

    /// Output collection: mapped material tracks
    std::string mappedMaterialCollection = "mapped_material_tracks";

    /// Output collection: un-mapped material tracks
    std::string unmappedMaterialCollection = "unmapped_material_tracks";

    /// The mappers which
    std::shared_ptr<Acts::IMaterialMapper> materialMapper = nullptr;

    /// The writer of the material
    std::vector<std::shared_ptr<IMaterialWriter>> materialWriters{};
  };

  /// Constructor
  ///
  /// @param cfg The configuration struct carrying the used tools
  /// @param level The output logging level
  MaterialMapping(const Config& cfg,
                  Acts::Logging::Level level = Acts::Logging::INFO);

  /// Destructor
  /// - it also writes out the file
  ~MaterialMapping() override;

  /// Framework execute method
  ///
  /// @param context The algorithm context for event consistency
  ActsExamples::ProcessCode execute(
      const AlgorithmContext& context) const override;

 private:
  Config m_cfg;

  std::unique_ptr<Acts::IMaterialMapper::State> m_state = nullptr;

  ReadDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_inputMaterialTracks{this, "InputMaterialTracks"};

  WriteDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_outputMaterialTracks{this, "OutputMaterialTracks"};

  WriteDataHandle<std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>>
      m_outputUnmappedMaterialTracks{this, "OutputUnmappedMaterialTracks"};
};

}  // namespace ActsExamples
