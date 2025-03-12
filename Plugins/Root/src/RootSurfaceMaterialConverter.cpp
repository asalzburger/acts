// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Root/RootSurfaceMaterialConverter.hpp"

#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Enumerate.hpp"

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
  // Remove the base tag
  std::string currentString = materialString;
  currentString.erase(0, cfg.baseTag.size());
  // split into tokens by the tags
  std::vector<std::string> tags = {cfg.volTag, cfg.bouTag, cfg.layTag,
                                   cfg.appTag, cfg.senTag, cfg.extraTag};
  std::vector<unsigned int> values;
  for (const auto [itag, tag] : Acts::enumerate(tags)) {
    currentString.erase(0, tag.size());
    // Find the tag
    std::size_t nextTag = (tag != tags.back())
                              ? currentString.find(tags[itag + 1u])
                              : currentString.size();
    // Extract the value
    values.push_back(std::stoi(currentString.substr(0, nextTag)));
    // Remove the value
    currentString.erase(0, nextTag);
  }

  // Return the geometry identifier
  return Acts::GeometryIdentifier()
      .withVolume(values[0])
      .withBoundary(values[1])
      .withLayer(values[2])
      .withApproach(values[3])
      .withSensitive(values[4])
      .withExtra(values[5]);
}
}  // namespace
void Acts::RootSurfaceMaterialConverter::toRoot(
    TDirectory& rootDir,
    const std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>&
        surfaceMaterialMap) {
  rootDir.cd();

  // Loop over the surface material maps with identifier
  for (const auto& [geoID, surfaceMaterial] : surfaceMaterialMap) {
    // Check if it is a homogenous surface material
    auto hsm = std::dynamic_pointer_cast<const HomogeneousSurfaceMaterial>(
        surfaceMaterial);
    if (hsm) {
      // Convert and Write the TObject to the ROOT directory
      toRoot(*hsm)->Write(encodeGeometryID(m_cfg, geoID).c_str());
      continue;
    }
    // Check if it is a binned surface material
    auto bsm =
        std::dynamic_pointer_cast<const BinnedSurfaceMaterial>(surfaceMaterial);
    if (bsm) {
      // Convert and Write the TObject to the ROOT directory
      toRoot(geoID, *bsm)->Write(encodeGeometryID(m_cfg, geoID).c_str());
      continue;
    }
  }
}

std::unique_ptr<TObject> Acts::RootSurfaceMaterialConverter::toRoot(
    const HomogeneousSurfaceMaterial& hsm) {
  const auto& materialSlab = hsm.materialSlab();
  const auto& material = materialSlab.material();
  // Create a new TObject
  auto tObj = std::make_unique<TVectorT<float>>(8);
  (*tObj)(0u) = material.X0();
  (*tObj)(1u) = material.L0();
  (*tObj)(2u) = material.Ar();
  (*tObj)(3u) = material.Z();
  (*tObj)(4u) = material.molarDensity();
  (*tObj)(5u) = material.molarElectronDensity();
  (*tObj)(6u) = material.meanExcitationEnergy();
  (*tObj)(7u) = materialSlab.thickness();
  // Return the TObject
  return tObj;
}

std::tuple<Acts::GeometryIdentifier,
           std::shared_ptr<const Acts::HomogeneousSurfaceMaterial>>
Acts::RootSurfaceMaterialConverter::fromRoot(
    const std::string& name, const TVectorT<float>& rootRep) const {
  // Decode the geometry identifier
  GeometryIdentifier geoID = decodeGeometryID(m_cfg, name);
  // Create the material
  auto material =
      Material::fromMolarDensity(rootRep(0), rootRep(1), rootRep(2), rootRep(3),
                                 rootRep(4), rootRep(5), rootRep(6));
  auto materialSlab = MaterialSlab(material, rootRep(7));
  // Create the homogeneous surface material
  auto hsm = std::make_shared<HomogeneousSurfaceMaterial>(materialSlab, 1.);
  // Return the geometry identifier and the surface material
  return std::make_tuple(geoID, hsm);
}

std::unique_ptr<TObject> Acts::RootSurfaceMaterialConverter::toRoot(
    const GeometryIdentifier& geoID, const BinnedSurfaceMaterial& bsm) {
  // Encode the name
  std::string name = encodeGeometryID(m_cfg, geoID);
  const auto& bUtility = bsm.binUtility();

  // x-y binning is the actual bin structure
  Int_t nBinsX = bUtility.bins(0u);
  Float_t xMin = bUtility.binningData()[0u].min;
  Float_t xMax = bUtility.binningData()[0u].max;
  std::string xAxisDir = axisDirectionName(bUtility.binningData()[0u].binvalue);

  Int_t nBinsY = bUtility.dimensions() > 1 ? bUtility.bins(1u) : 1;
  Float_t yMin = nBinsY > 1 ? bUtility.binningData()[1u].min : 0;
  Float_t yMax = nBinsY > 1 ? bUtility.binningData()[1u].max : 1;
  std::string yAxisDir =
      nBinsY > 1 ? axisDirectionName(bUtility.binningData()[1u].binvalue)
                 : "N/A";

  // 8 bins in z are : X0, L0, Ar, A, molarDensity, molarElectronDensity,
  // meanExcitationEnergy, thickness
  auto tObj = std::make_unique<TH3F>(name.c_str(), name.c_str(), nBinsX, xMin,
                                     xMax, nBinsY, yMin, yMax, 8, 0., 8.);
  tObj->GetXaxis()->SetTitle(xAxisDir.c_str());
  tObj->GetYaxis()->SetTitle(yAxisDir.c_str());
  tObj->GetZaxis()->SetTitle("Material Properties");

  // Get the full material matrix
  const auto& materialMatrix = bsm.fullMaterial();
  // Loop over the bins and fill
  for (const auto [imv, materialVector] : enumerate(materialMatrix)) {
    for (const auto [imm, materialSlab] : enumerate(materialVector)) {
      // Fill the bin
      const auto& material = materialSlab.material();
      tObj->SetBinContent(imm + 1, imv + 1, 1, material.X0());
      tObj->SetBinContent(imm + 1, imv + 1, 2, material.L0());
      tObj->SetBinContent(imm + 1, imv + 1, 3, material.Ar());
      tObj->SetBinContent(imm + 1, imv + 1, 4, material.Z());
      tObj->SetBinContent(imm + 1, imv + 1, 5, material.molarDensity());
      tObj->SetBinContent(imm + 1, imv + 1, 6, material.molarElectronDensity());
      tObj->SetBinContent(imm + 1, imv + 1, 7, material.meanExcitationEnergy());
      tObj->SetBinContent(imm + 1, imv + 1, 8, materialSlab.thickness());
    }
  }
  // Return the TObject
  return tObj;
}

std::tuple<Acts::GeometryIdentifier,
           std::shared_ptr<const Acts::BinnedSurfaceMaterial>>
Acts::RootSurfaceMaterialConverter::fromRoot(const TH3F& rootRep) const {
  std::string name = rootRep.GetName();
  // Decode the geometry identifier
  GeometryIdentifier geoID = decodeGeometryID(m_cfg, name);
  // Get the binning
  auto bUtility =
      BinUtility(rootRep.GetNbinsX(), rootRep.GetXaxis()->GetXmin(),
                 rootRep.GetXaxis()->GetXmax(), open,
                 axisDirectionFromName(rootRep.GetXaxis()->GetTitle()));
  // Add the y binning if available
  if (rootRep.GetNbinsY() > 1) {
    bUtility +=
        BinUtility(rootRep.GetNbinsY(), rootRep.GetYaxis()->GetXmin(),
                   rootRep.GetYaxis()->GetXmax(), open,
                   axisDirectionFromName(rootRep.GetYaxis()->GetTitle()));
  }
  // Create the material matrix
  std::vector<std::vector<MaterialSlab>> materialMatrix;
  for (Int_t imv = 0; imv < rootRep.GetNbinsY(); ++imv) {
    std::vector<MaterialSlab> materialVector;
    for (Int_t imm = 0; imm < rootRep.GetNbinsX(); ++imm) {
      // Create the material
      Material material = Material::fromMolarDensity(
          rootRep.GetBinContent(imm + 1, imv + 1, 1),
          rootRep.GetBinContent(imm + 1, imv + 1, 2),
          rootRep.GetBinContent(imm + 1, imv + 1, 3),
          rootRep.GetBinContent(imm + 1, imv + 1, 4),
          rootRep.GetBinContent(imm + 1, imv + 1, 5),
          rootRep.GetBinContent(imm + 1, imv + 1, 6),
          rootRep.GetBinContent(imm + 1, imv + 1, 7));
      float thickness = rootRep.GetBinContent(imm + 1, imv + 1, 8);
      MaterialSlab materialSlab(material, thickness);
      // Add the material slab
      materialVector.push_back(materialSlab);
    }
    // Add the material vector
    materialMatrix.push_back(materialVector);
  }
  // Create the binned surface material
  auto bsm = std::make_shared<BinnedSurfaceMaterial>(bUtility,
                                                     std::move(materialMatrix));
  return std::tie(geoID, bsm);
}
