// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Root/RootSurfaceMaterialConverter.hpp"

namespace {
/// Encode the Geometry ID into a string
/// @param cfg The configuration
/// @param geoID The geometry identifier
/// @return The encoded string
std::string encodeGeometryID(
    const Acts::RootSurfaceMaterialConverter::Config& cfg,
    const Acts::GeometryIdentifier& geoID) {
  // Create a string stream
  std::stringstream ss;
  // Write the volume ID
  ss << cfg.baseTag;
  ss << cfg.volTag << geoID.volume();
  ss << cfg.bouTag << geoID.boundary();
  ss << cfg.layTag << geoID.layer();
  ss << cfg.appTag << geoID.approach();
  ss << cfg.senTag << geoID.sensitive();
  ss << cfg.extraTag << geoID.extra();
  // Return the string
  return ss.str();
}

// Decode the Geometry ID from a string
/// @param cfg The configuration
/// @param materialString
/// @return The geometry identifier
Acts::GeometryIdentifier decodeGeometryID(
    const Acts::RootSurfaceMaterialConverter::Config& cfg,
    const std::string& materialString) {
  Acts::GeometryIdentifier geoID;
  // Remove the base tag
  std::string currentString = materialString;
  currentString.erase(0, cfg.baseTag.size());
  // Remove the volume tag
  std::string volumeString =
      currentString.substr(0, currentString.find(cfg.volTag));
  geoID = geoID.withVolume(std::stoul(volumeString));
  currentString.erase(0, volumeString.size() + cfg.volTag.size());
  // Remove the boundary tag
  std::string boundaryString =
      currentString.substr(0, currentString.find(cfg.bouTag));
  geoID = geoID.withBoundary(std::stoul(boundaryString));
  currentString.erase(0, boundaryString.size() + cfg.bouTag.size());
  // Remove the layer tag
  std::string layerString =
      currentString.substr(0, currentString.find(cfg.layTag));
  geoID = geoID.withLayer(std::stoul(layerString));
  currentString.erase(0, layerString.size() + cfg.layTag.size());
  // Remove the approach tag
  std::string approachString =
      currentString.substr(0, currentString.find(cfg.appTag));
  geoID = geoID.withApproach(std::stoul(approachString));
  currentString.erase(0, approachString.size() + cfg.appTag.size());
  // Remove the sensitive tag
  std::string sensitiveString =
      currentString.substr(0, currentString.find(cfg.senTag));
  geoID = geoID.withSensitive(std::stoul(sensitiveString));
  currentString.erase(0, sensitiveString.size() + cfg.senTag.size());
  // Remove the extra tag
  std::string extraString =
      currentString.substr(0, currentString.find(cfg.extraTag));
  geoID = geoID.withExtra(std::stoul(extraString));
  currentString.erase(0, extraString.size() + cfg.extraTag.size());
  // Return the geometry identifier
  return geoID;
}
}  // namespace

std::unique_ptr<TDirectory> Acts::RootSurfaceMaterialConverter::toRoot(
    const std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>&
        surfaceMaterialMaps) {
  // Create a new ROOT directory
  auto rootDir = std::make_unique<TDirectory>(m_cfg.directoryName.c_str(),
                                              m_cfg.directoryName.c_str());
  // Loop over the surface material maps with identifier
  for (const auto& [geoID, surfaceMaterial] : surfaceMaterialMaps) {

}

  // Return the ROOT directory
  return rootDir;
}