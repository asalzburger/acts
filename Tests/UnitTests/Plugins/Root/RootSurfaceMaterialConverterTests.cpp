// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Plugins/Root/RootSurfaceMaterialConverter.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <iostream>
#include <string>
#include <tuple>

#include <TFile.h>
#include <TH3F.h>
#include <TTree.h>

using namespace Acts;

namespace {
// Helper method to compare material matrices
bool compareMaterialMatrices(std::vector<std::vector<Acts::MaterialSlab>> m1,
                             std::vector<std::vector<Acts::MaterialSlab>> m2) {
  BOOST_CHECK_EQUAL(m1.size(), m2.size());
  for (size_t i = 0; i < m1.size(); ++i) {
    BOOST_CHECK_EQUAL(m1[i].size(), m2[i].size());
    for (size_t j = 0; j < m1[i].size(); ++j) {
      CHECK_CLOSE_ABS(m1[i][j].material().X0(), m2[i][j].material().X0(),
                            1e-6);
      CHECK_CLOSE_ABS(m1[i][j].material().L0(), m2[i][j].material().L0(),
                            1e-6);
      CHECK_CLOSE_ABS(m1[i][j].material().Ar(), m2[i][j].material().Ar(),
                            1e-6);
      CHECK_CLOSE_ABS(m1[i][j].material().Z(), m2[i][j].material().Z(),
                            1e-6);
      CHECK_CLOSE_ABS(m1[i][j].material().molarDensity(),

                            m2[i][j].material().molarDensity(), 1e-6);
      CHECK_CLOSE_ABS(m1[i][j].material().molarElectronDensity(),
                            m2[i][j].material().molarElectronDensity(), 1e-6);
      CHECK_CLOSE_ABS(m1[i][j].material().meanExcitationEnergy(),
                            m2[i][j].material().meanExcitationEnergy(), 1e-6);
      CHECK_CLOSE_ABS(m1[i][j].thickness(), m2[i][j].thickness(), 1e-6);
    }
  }
  return true;
}
}  // namespace

// Create the converter
Acts::RootSurfaceMaterialConverter::Config rsmcConfig;
Acts::RootSurfaceMaterialConverter rsmc(rsmcConfig);

// Create a map of surface materials
std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
    surfaceMaterialMap;

BOOST_AUTO_TEST_SUITE(RootPlugin)

BOOST_AUTO_TEST_CASE(RootHomogenousSurfaceMaterialConversion) {
  // (A) Create a homogeneous surface material
  // Construct the material properties from arguments
  Material mat = Acts::Material::fromMolarDensity(100., 33., 14., 7., 0.3);
  MaterialSlab mp(mat, 1);

  auto hsm = std::make_shared<HomogeneousSurfaceMaterial>(mp, 1.);
  auto hsmId = GeometryIdentifier()
                   .withVolume(1)
                   .withBoundary(2)
                   .withLayer(3)
                   .withApproach(4)
                   .withSensitive(5)
                   .withExtra(6);
  std::string hsmIdString = "surface_material_vol1_bou2_lay3_app4_sen5_extra6";

  // (1) Convert to ROOT
  auto hsmAsTObject = rsmc.toRoot(*hsm);
  BOOST_CHECK(hsmAsTObject != nullptr);

  // (2) Convert from ROOT
  auto [hsmIDIn, hsmTObjIn] = rsmc.fromRoot(
      hsmIdString, *dynamic_cast<TVectorT<float>*>(hsmAsTObject.get()));
  BOOST_CHECK(hsmIDIn == hsmId);
  BOOST_CHECK(hsmTObjIn != nullptr);

  auto hsmIn =
      std::dynamic_pointer_cast<const HomogeneousSurfaceMaterial>(hsmTObjIn);
  BOOST_CHECK(hsmIn != nullptr);

  CHECK_CLOSE_ABS(hsmIn->materialSlab().material().X0(), mat.X0(), 1e-6);
  CHECK_CLOSE_ABS(hsmIn->materialSlab().material().L0(), mat.L0(), 1e-6);
  CHECK_CLOSE_ABS(hsmIn->materialSlab().material().Ar(), mat.Ar(), 1e-6);
  CHECK_CLOSE_ABS(hsmIn->materialSlab().material().Z(), mat.Z(), 1e-6);
  CHECK_CLOSE_ABS(hsmIn->materialSlab().material().molarDensity(),
                  mat.molarDensity(), 1e-6);
  CHECK_CLOSE_ABS(hsmIn->materialSlab().material().molarElectronDensity(),
                  mat.molarElectronDensity(), 1e-6);
  CHECK_CLOSE_ABS(hsmIn->materialSlab().material().meanExcitationEnergy(),
                  mat.meanExcitationEnergy(), 1e-6);
  CHECK_CLOSE_ABS(hsmIn->materialSlab().thickness(),
                  hsm->materialSlab().thickness(), 1e-6);

  // Add to maps
  surfaceMaterialMap[hsmId] = hsm;
}

BOOST_AUTO_TEST_CASE(RootBinnedSurfaceMaterialConversion) {
  // (B) Create binned surface material

  // Constructor a few material properties
  MaterialSlab a00(Material::fromMolarDensity(1., 2., 3., 4., 5.,6.,7.), 8.);
  MaterialSlab a01(Material::fromMolarDensity(2., 3., 4., 5., 6., 7., 8.), 9.);
  MaterialSlab a02(Material::fromMolarDensity(3., 4., 5., 6., 7., 8., 9.), 10.);
  MaterialSlab a10(Material::fromMolarDensity(4., 5., 6., 7., 8., 9., 10), 11.);
  MaterialSlab a11(Material::fromMolarDensity(5., 6., 7., 8., 9., 10., 11.), 12.);
  MaterialSlab a12(Material::fromMolarDensity(6., 7., 8., 9., 10., 11., 12.), 13.);

  // (B1) one-dimension binning
  BinUtility rBinning(5, 1., 10., open, AxisDirection::AxisR);
  std::vector<MaterialSlab> l01 = {a00, a01, a02, a10, a11};
  std::vector<std::vector<MaterialSlab>> m1 = {std::move(l01)};

  // Create the material - don't move the material matrix, we want is as
  // reference
  auto bsm1 = std::make_shared<BinnedSurfaceMaterial>(rBinning, m1);
  auto bsmId1 = GeometryIdentifier()
                    .withVolume(10)
                    .withBoundary(2)
                    .withLayer(12)
                    .withApproach(5)
                    .withSensitive(14)
                    .withExtra(16);
  std::string bsmIdString1 =
      "surface_material_vol10_bou2_lay12_app5_sen14_extra16";

  // (1) Convert to ROOT
  auto bsmAsTObject1 = rsmc.toRoot(bsmId1, *bsm1);

  // (2) Convert from ROOT
  auto [bsmIDIn1, bsmTObjIn1] =
      rsmc.fromRoot(*(dynamic_cast<TH3F*>(bsmAsTObject1.get())));
  BOOST_CHECK(bsmIDIn1 == bsmId1);
  BOOST_CHECK(bsmTObjIn1 != nullptr);

  auto bsmIn1 =
      std::dynamic_pointer_cast<const BinnedSurfaceMaterial>(bsmTObjIn1);
  BOOST_CHECK(bsmIn1 != nullptr);
  const auto& m1In = bsmIn1->fullMaterial();
  BOOST_CHECK(compareMaterialMatrices(m1In, m1));

  surfaceMaterialMap[bsmId1] = bsm1;

  // (B2) two-dimension binning
  BinUtility xyBinning(2, -1., 1., open, AxisDirection::AxisX);
  xyBinning += BinUtility(3, -3., 3., open, AxisDirection::AxisY);

  // Prepare the matrix
  std::vector<MaterialSlab> l0 = {a00, a10};
  std::vector<MaterialSlab> l1 = {a01, a11};
  std::vector<MaterialSlab> l2 = {a02, a12};

  // Build the matrix
  std::vector<std::vector<MaterialSlab>> m2 = {std::move(l0), std::move(l1),
                                               std::move(l2)};

  // Create the material - don't move the material matrix, we need it as
  // reference
  auto bsm2 = std::make_shared<BinnedSurfaceMaterial>(xyBinning, m2);
  auto bsmId2 = GeometryIdentifier()
                    .withVolume(10)
                    .withBoundary(1)
                    .withLayer(12)
                    .withApproach(3)
                    .withSensitive(14)
                    .withExtra(5);
  std::string bsmIdString2 =
      "surface_material_vol10_bou1_lay12_app3_sen14_extra5";

  // (1) Convert to ROOT
  auto bsmAsTObject2 = rsmc.toRoot(bsmId2, *bsm2);

  // (2) Convert from ROOT
  auto [bsmIDIn2, bsmTObjIn2] =
      rsmc.fromRoot(*(dynamic_cast<TH3F*>(bsmAsTObject2.get())));
  BOOST_CHECK(bsmIDIn2 == bsmId2);
  BOOST_CHECK(bsmTObjIn2 != nullptr);

  auto bsmIn2 =
      std::dynamic_pointer_cast<const BinnedSurfaceMaterial>(bsmTObjIn2);
  BOOST_CHECK(bsmIn2 != nullptr);
  const auto& m2In = bsmIn2->fullMaterial();
  BOOST_CHECK(compareMaterialMatrices(m2In, m2));
  // Store for later checks
  surfaceMaterialMap[bsmId2] = bsm2;
}

BOOST_AUTO_TEST_CASE(RootSurfaceMaterialMapConversion) {
  auto rFile =
      TFile::Open("RootSurfaceMaterialConverterTests.root", "RECREATE");
  rsmc.toRoot(*rFile, surfaceMaterialMap);
  rFile->Close();
}

BOOST_AUTO_TEST_SUITE_END()
