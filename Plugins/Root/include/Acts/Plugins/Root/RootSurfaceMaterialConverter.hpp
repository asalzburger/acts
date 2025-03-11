// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"

#include <map>
#include <memory>

#include <TDirectory.h>

namespace Acts {

class RootSurfaceMaterialConverter {
 public:
  struct Config {
    // The name of the ROOT directory
    std::string directoryName = "SurfaceMaterialMaps";

    /// The base tag
    std::string baseTag = "surface_material";
    /// The volume identification string
    std::string volTag = "_vol";
    /// The boundary identification string
    std::string bouTag = "_bou";
    /// The layer identification string
    std::string layTag = "_lay";
    /// The approach identification string
    std::string appTag = "_app";
    /// The sensitive identification string
    std::string senTag = "_sen";
    /// The potential extra tag
    std::string extraTag = "_extra";
  };

  /// @brief Constructor
  RootSurfaceMaterialConverter(const Config& cfg) : m_cfg(cfg) {}

  /// @brief Convert the surface material maps to a ROOT directory
  ///
  /// @param surfaceMaterialMaps The surface material maps
  ///
  /// This will translate the surface material map to a ROOT directory
  /// which then can be added to a ROOT file.
  std::unique_ptr<TDirectory> toRoot(
      const std::map<GeometryIdentifier,
                     std::shared_ptr<const ISurfaceMaterial>>&
          surfaceMaterialMaps);

 private:
  Config m_cfg;
};
}  // namespace Acts