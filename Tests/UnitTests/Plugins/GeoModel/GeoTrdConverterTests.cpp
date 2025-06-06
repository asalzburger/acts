// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoTrd.h>

Acts::GeometryContext tContext;
Acts::RotationMatrix3 idRotation = Acts::RotationMatrix3::Identity();
Acts::Transform3 idTransform = Acts::Transform3::Identity();

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

// GeoBox conversion test case
BOOST_AUTO_TEST_CASE(GeoTrfToSensitiveConversion) {
  auto material = make_intrusive<GeoMaterial>("Material", 1.0);
  // Let's create a GeoFullPhysVol object

  // (Trapezoid object) - YZ
  //  GeoTrd (double XHalfLength1, double XHalfLength2, double YHalfLength1,
  //  double YHalfLength2, double ZHalfLength);
  auto trapYZ = make_intrusive<GeoTrd>(2, 2, 50, 80, 60);
  auto logYZ = make_intrusive<GeoLogVol>("LogVolumeYZ", trapYZ, material);
  auto fphysYZ = make_intrusive<GeoFullPhysVol>(logYZ);

  PVConstLink physYZ{make_intrusive<GeoFullPhysVol>(logYZ)};

  Acts::SurfaceBoundFactory boundFactory{};
  auto converted = Acts::GeoTrdConverter{}.toSensitiveSurface(
      physYZ, idTransform, boundFactory);

  BOOST_CHECK(converted.ok());

  auto [elementYZ, surfaceYZ] = converted.value();

  // Check the bounds
  const Acts::TrapezoidBounds* tBoundsYZ =
      dynamic_cast<const Acts::TrapezoidBounds*>(&(surfaceYZ->bounds()));

  BOOST_CHECK(tBoundsYZ != nullptr);
  CHECK_CLOSE_ABS(
      tBoundsYZ->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXnegY), 50,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsYZ->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXposY), 80,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsYZ->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthY), 60,
      1e-6);

  // Check the transform -> should be cyclic permutation of the identity
  const Acts::Transform3& transformYZ = surfaceYZ->transform(tContext);

  Acts::RotationMatrix3 rotationYZ = transformYZ.rotation();
  BOOST_CHECK(rotationYZ.col(0).isApprox(idRotation.col(1)));
  BOOST_CHECK(rotationYZ.col(1).isApprox(idRotation.col(2)));
  BOOST_CHECK(rotationYZ.col(2).isApprox(idRotation.col(0)));

  // (Trapezoid object) - YZ swapped
  //  GeoTrd (double XHalfLength1, double XHalfLength2, double YHalfLength1,
  //  double YHalfLength2, double ZHalfLength);
  auto trapYZs = make_intrusive<GeoTrd>(2, 2, 80, 50, 60);
  auto logYZs = make_intrusive<GeoLogVol>("LogVolumeYZs", trapYZs, material);
  auto fphysYZs = make_intrusive<GeoFullPhysVol>(logYZs);

  converted = Acts::GeoTrdConverter{}.toSensitiveSurface(fphysYZs, idTransform,
                                                         boundFactory);

  BOOST_CHECK(converted.ok());

  auto [elementYZs, surfaceYZs] = converted.value();

  // Check the bounds
  const Acts::TrapezoidBounds* tBoundsYZs =
      dynamic_cast<const Acts::TrapezoidBounds*>(&(surfaceYZs->bounds()));

  BOOST_CHECK(tBoundsYZs != nullptr);
  CHECK_CLOSE_ABS(
      tBoundsYZs->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXnegY), 50,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsYZs->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXposY), 80,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsYZs->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthY), 60,
      1e-6);

  // Check the transform -> should be cyclic permutation of the identity
  const Acts::Transform3& transformYZs = surfaceYZs->transform(tContext);

  Acts::RotationMatrix3 rotationYZs = transformYZs.rotation();
  BOOST_CHECK(rotationYZs.col(0).isApprox(idRotation.col(1)));
  BOOST_CHECK(rotationYZs.col(1).isApprox(-idRotation.col(2)));
  BOOST_CHECK(rotationYZs.col(2).isApprox(-idRotation.col(0)));

  // (Trapezoid object) - XZ
  auto trapXZ = make_intrusive<GeoTrd>(50, 80, 2, 2, 60);
  auto logXZ = make_intrusive<GeoLogVol>("LogVolumeXZ", trapXZ, material);
  auto fphysXZ = make_intrusive<GeoFullPhysVol>(logXZ);

  converted = Acts::GeoTrdConverter{}.toSensitiveSurface(fphysXZ, idTransform,
                                                         boundFactory);

  BOOST_CHECK(converted.ok());

  auto [elementXZ, surfaceXZ] = converted.value();

  // Check the bounds
  const Acts::TrapezoidBounds* tBoundsXZ =
      dynamic_cast<const Acts::TrapezoidBounds*>(&(surfaceXZ->bounds()));

  BOOST_CHECK(tBoundsXZ != nullptr);
  CHECK_CLOSE_ABS(
      tBoundsXZ->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXnegY), 50,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsXZ->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXposY), 80,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsXZ->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthY), 60,
      1e-6);

  // Check the transform -> cyclic permuttation not possible
  const Acts::Transform3& transformXZ = surfaceXZ->transform(tContext);

  Acts::RotationMatrix3 rotationXZ = transformXZ.rotation();
  BOOST_CHECK(rotationXZ.col(0).isApprox(idRotation.col(0)));
  BOOST_CHECK(rotationXZ.col(1).isApprox(idRotation.col(2)));
  BOOST_CHECK(rotationXZ.col(2).isApprox(-1 * idRotation.col(1)));

  // (Trapezoid object) - XZs (swapped)
  auto trapXZs = make_intrusive<GeoTrd>(80, 50, 2, 2, 60);
  auto logXZs = make_intrusive<GeoLogVol>("LogVolumeXZs", trapXZs, material);
  auto fphysXZs = make_intrusive<GeoFullPhysVol>(logXZs);

  PVConstLink physXZs{make_intrusive<GeoFullPhysVol>(logXZs)};

  converted = Acts::GeoTrdConverter{}.toSensitiveSurface(physXZs, idTransform,
                                                         boundFactory);

  BOOST_CHECK(converted.ok());

  auto [elementXZs, surfaceXZs] = converted.value();

  // Check the bounds
  const Acts::TrapezoidBounds* tBoundsXZs =
      dynamic_cast<const Acts::TrapezoidBounds*>(&(surfaceXZs->bounds()));

  BOOST_CHECK(tBoundsXZs != nullptr);
  CHECK_CLOSE_ABS(
      tBoundsXZs->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXnegY), 50,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsXZs->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthXposY), 80,
      1e-6);
  CHECK_CLOSE_ABS(
      tBoundsXZs->get(Acts::TrapezoidBounds::BoundValues::eHalfLengthY), 60,
      1e-6);

  // Check the transform -> cyclic permuttation not possible
  const Acts::Transform3& transformXZs = surfaceXZs->transform(tContext);

  Acts::RotationMatrix3 rotationXZs = transformXZs.rotation();
  BOOST_CHECK(rotationXZs.col(0).isApprox(idRotation.col(0)));
  BOOST_CHECK(rotationXZs.col(1).isApprox(-idRotation.col(2)));
  BOOST_CHECK(rotationXZs.col(2).isApprox(idRotation.col(1)));

  // Double - trazoid -> throw exception
  auto trapDouble = make_intrusive<GeoTrd>(50, 80, 50, 80, 60);
  auto logDouble =
      make_intrusive<GeoLogVol>("LogVolumeDouble", trapDouble, material);
  auto fphysDouble = make_intrusive<GeoFullPhysVol>(logDouble);

  BOOST_CHECK_THROW(Acts::GeoTrdConverter{}.toSensitiveSurface(
                        fphysDouble, idTransform, boundFactory),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
