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
#include <string>
#include <tuple>

#include <TDirectory.h>
#include <TObject.h>
#include <TH3F.h>
#include <TVectorT.h>

namespace Acts {

class HomogeneousSurfaceMaterial;
class BinnedSurfaceMaterial;

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
  /// @param cfg is the configuration struct
  explicit RootSurfaceMaterialConverter(const Config& cfg) : m_cfg(cfg) {}

  /// @brief Convert the surface material maps to a ROOT directory
  ///
  /// @param rootDir The ROOT directory to which the surface material maps
  /// @param surfaceMaterialMap The map of surface material objects
  ///
  /// This will translate the surface material map to a ROOT directory
  /// which then can be added to a ROOT file.
  void toRoot(TDirectory& rootDir,
      const std::map<GeometryIdentifier,
                     std::shared_ptr<const ISurfaceMaterial>>&
          surfaceMaterialMap);

  /// @brief Convert a homogenous surface material to a TObject
  /// @param hsm the homogenous surface material
  /// @return a unique pointer to the TObject
  std::unique_ptr<TObject> toRoot(const HomogeneousSurfaceMaterial& hsm);

  /// @brief Convert from TObject to a homogenous surface material
  /// @param name the name of the object
  /// @param rootRep the root representation
  /// @return a pair of the geometry identifier and the surface material
  std::tuple<GeometryIdentifier, std::shared_ptr<const HomogeneousSurfaceMaterial>>
  fromRoot(const std::string& name, const TVectorT<float>& rootRep) const;

  /// @brief Convert a binned surface material to a TObject
  /// @param geoID the geometry identifier
  /// @param bsm the binned surface material
  /// @return a unique pointer to the TObject
  std::unique_ptr<TObject> toRoot(
    const GeometryIdentifier& geoID,
    const BinnedSurfaceMaterial& bsm);

  /// @brief Convert from TObject to a binned surface material
  /// @param rootRep the root representation
  /// @note the root object has the name encoded already
  /// @return a pair of the geometry identifier and the surface material
  std::tuple<GeometryIdentifier, std::shared_ptr<const BinnedSurfaceMaterial>>
  fromRoot(const TH3F& rootRep) const;



 private:
  Config m_cfg;
};
}  // namespace Acts