// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/detail/log_level.hpp>
#include <boost/test/tools/context.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_parameters.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Geometry/VolumeResizeStrategy.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Zip.hpp"

#include <numbers>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

auto logger = Acts::getDefaultLogger("UnitTests", Acts::Logging::VERBOSE);

struct Fixture {
  Logging::Level m_level;
  Fixture() {
    m_level = Acts::Logging::getFailureThreshold();
    Acts::Logging::setFailureThreshold(Acts::Logging::FATAL);
  }

  ~Fixture() { Acts::Logging::setFailureThreshold(m_level); }
};

BOOST_FIXTURE_TEST_SUITE(Geometry, Fixture)

static const std::vector<VolumeAttachmentStrategy> strategies = {
    VolumeAttachmentStrategy::Gap,
    VolumeAttachmentStrategy::First,
    VolumeAttachmentStrategy::Second,
    VolumeAttachmentStrategy::Midpoint,
};

static const std::vector<VolumeResizeStrategy> resizeStrategies = {
    VolumeResizeStrategy::Expand,
    VolumeResizeStrategy::Gap,
};

BOOST_AUTO_TEST_SUITE(CylinderVolumeStackTest)
BOOST_AUTO_TEST_SUITE(ZDirection)

BOOST_DATA_TEST_CASE(Baseline,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::xrange(0, 2, 1) *
                      boost::unit_test::data::make(0.8, 1.0, 1.2) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(strategies)),
                     angle, rotate, shift, offset, strategy) {
  double hlZ = 400_mm;

  // Cylinder volumes which already line up, but have different1 radii
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(300_mm, 500_mm, hlZ);

  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  Transform3 transform1 = base;
  transform1.translate(Vector3{0_mm, 0_mm, -2 * hlZ * shift});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  transform2.translate(Vector3{0_mm, 0_mm, 0_mm});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = base;
  transform3.translate(Vector3{0_mm, 0_mm, 2 * hlZ * shift});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Rotate to simulate unsorted volumes: all results should be the same!
  std::rotate(volumes.begin(), volumes.begin() + rotate, volumes.end());

  auto origVolumes = volumes;

  std::vector<CylinderVolumeBounds> originalBounds;
  std::transform(
      volumes.begin(), volumes.end(), std::back_inserter(originalBounds),
      [](const auto& vol) {
        return *dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
      });

  if (shift < 1.0) {
    BOOST_CHECK_THROW(
        CylinderVolumeStack(volumes, AxisDirection::AxisZ, strategy,
                            VolumeResizeStrategy::Gap, *logger),
        std::invalid_argument);
    return;
  }
  CylinderVolumeStack cylStack(volumes, AxisDirection::AxisZ, strategy,
                               VolumeResizeStrategy::Gap, *logger);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    hlZ + 2 * hlZ * shift);
  CHECK_CLOSE_OR_SMALL(cylStack.transform().matrix(), base.matrix(), 1e-10,
                       1e-14);

  // All volumes (including gaps) are cylinders and have the same radial bounds
  for (const auto& volume : volumes) {
    const auto* cylinderBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
    BOOST_REQUIRE(cylinderBounds != nullptr);
    BOOST_CHECK_EQUAL(cylinderBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylinderBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
  }

  // Volumes are sorted in (local) z
  for (std::size_t i = 0; i < volumes.size() - 1; ++i) {
    const auto& a = volumes.at(i);
    const auto& b = volumes.at(i + 1);

    BOOST_CHECK_LT((base.inverse() * a->center())[eZ],
                   (base.inverse() * b->center())[eZ]);
  }

  if (shift <= 1.0) {
    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // No expansion, original volumes did not move
    BOOST_CHECK_EQUAL(vol1->transform().matrix(), transform1.matrix());
    BOOST_CHECK_EQUAL(vol2->transform().matrix(), transform2.matrix());
    BOOST_CHECK_EQUAL(vol3->transform().matrix(), transform3.matrix());

    for (const auto& [volume, bounds] : zip(origVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        bounds.get(CylinderVolumeBounds::eHalfLengthZ));
    }
  } else {
    if (strategy == VolumeAttachmentStrategy::Gap) {
      // Gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);
      auto gap1 = volumes.at(1);
      auto gap2 = volumes.at(3);

      BOOST_TEST_MESSAGE("Gap 1: " << gap1->transform().matrix());
      BOOST_TEST_MESSAGE("Gap 2: " << gap2->transform().matrix());

      const auto* gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());

      double gapHlZ = (shift - 1.0) * hlZ;

      BOOST_CHECK(std::abs(gapBounds1->get(CylinderVolumeBounds::eHalfLengthZ) -
                           gapHlZ) < 1e-10);
      BOOST_CHECK(std::abs(gapBounds2->get(CylinderVolumeBounds::eHalfLengthZ) -
                           gapHlZ) < 1e-10);

      double gap1Z = (-2 * hlZ * shift) + hlZ + gapHlZ;
      double gap2Z = (2 * hlZ * shift) - hlZ - gapHlZ;

      Transform3 gap1Transform = base * Translation3{0_mm, 0_mm, gap1Z};
      Transform3 gap2Transform = base * Translation3{0_mm, 0_mm, gap2Z};

      CHECK_CLOSE_OR_SMALL(gap1->transform().matrix(), gap1Transform.matrix(),
                           1e-10, 1e-14);
      CHECK_CLOSE_OR_SMALL(gap2->transform().matrix(), gap2Transform.matrix(),
                           1e-10, 1e-14);

      // Original volumes did not changes bounds
      for (const auto& [volume, bounds] : zip(origVolumes, originalBounds)) {
        const auto* newBounds =
            dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                          bounds.get(CylinderVolumeBounds::eHalfLengthZ));
      }

      // No expansion, original volumes did not move
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), transform1.matrix());
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), transform2.matrix());
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), transform3.matrix());

    } else if (strategy == VolumeAttachmentStrategy::First) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      double wGap = (shift - 1.0) * hlZ * 2;

      // Volume 1 got bigger and shifted right
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      double pZ1 = -2 * hlZ * shift + wGap / 2.0;
      Transform3 expectedTransform1 = base * Translation3{0_mm, 0_mm, pZ1};
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-14);

      // Volume 2 got bigger and shifted left
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      double pZ2 = wGap / 2.0;
      Transform3 expectedTransform2 = base * Translation3{0_mm, 0_mm, pZ2};
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(),
                           expectedTransform2.matrix(), 1e-10, 1e-14);

      // Volume 3 stayed the same
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      double pZ3 = 2 * hlZ * shift;
      Transform3 expectedTransform3 = base * Translation3{0_mm, 0_mm, pZ3};
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-14);
    } else if (strategy == VolumeAttachmentStrategy::Second) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      double wGap = (shift - 1.0) * hlZ * 2;

      // Volume 1 stayed the same
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      double pZ1 = -2 * hlZ * shift;
      Transform3 expectedTransform1 = base * Translation3{0_mm, 0_mm, pZ1};
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-14);

      // Volume 2 got bigger and shifted left
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      double pZ2 = -wGap / 2.0;
      Transform3 expectedTransform2 = base * Translation3{0_mm, 0_mm, pZ2};
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(),
                           expectedTransform2.matrix(), 1e-10, 1e-14);

      // Volume 3 got bigger and shifted left
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      double pZ3 = 2 * hlZ * shift - wGap / 2.0;
      Transform3 expectedTransform3 = base * Translation3{0_mm, 0_mm, pZ3};
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-14);
    } else if (strategy == VolumeAttachmentStrategy::Midpoint) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      double wGap = (shift - 1.0) * hlZ * 2;

      // Volume 1 got bigger and shifted right
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 4.0);
      double pZ1 = -2 * hlZ * shift + wGap / 4.0;
      Transform3 expectedTransform1 = base * Translation3{0_mm, 0_mm, pZ1};
      CHECK_CLOSE_OR_SMALL(vol1->transform().matrix(),
                           expectedTransform1.matrix(), 1e-10, 1e-14);

      // Volume 2 got bigger but didn't move
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 2.0);
      CHECK_CLOSE_OR_SMALL(vol2->transform().matrix(), base.matrix(), 1e-10,
                           1e-14);

      // Volume 3 got bigger and shifted left
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + wGap / 4.0);
      double pZ3 = 2 * hlZ * shift - wGap / 4.0;
      Transform3 expectedTransform3 = base * Translation3{0_mm, 0_mm, pZ3};
      CHECK_CLOSE_OR_SMALL(vol3->transform().matrix(),
                           expectedTransform3.matrix(), 1e-10, 1e-14);
    }
  }
}

BOOST_AUTO_TEST_CASE(Asymmetric) {
  double hlZ1 = 200_mm;
  double pZ1 = -1100_mm;
  double hlZ2 = 600_mm;
  double pZ2 = -200_mm;
  double hlZ3 = 400_mm;
  double pZ3 = 850_mm;

  // Cylinder volumes which already line up, but have different1 radii
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, hlZ1);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 600_mm, hlZ2);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(300_mm, 500_mm, hlZ3);

  Transform3 transform1 = Transform3::Identity();
  transform1.translate(Vector3{0_mm, 0_mm, pZ1});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = Transform3::Identity();
  transform2.translate(Vector3{0_mm, 0_mm, pZ2});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = Transform3::Identity();
  transform3.translate(Vector3{0_mm, 0_mm, pZ3});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol2.get(), vol1.get(), vol3.get()};

  CylinderVolumeStack cylStack(volumes, AxisDirection::AxisZ,
                               VolumeAttachmentStrategy::Gap,
                               VolumeResizeStrategy::Gap, *logger);
  BOOST_CHECK_EQUAL(volumes.size(), 5);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    (std::abs(pZ1 - hlZ1) + pZ3 + hlZ3) / 2.0);

  double midZ = (pZ1 - hlZ1 + pZ3 + hlZ3) / 2.0;
  Transform3 expectedTransform{Translation3{0_mm, 0_mm, midZ}};
  CHECK_CLOSE_OR_SMALL(cylStack.transform().matrix(),
                       expectedTransform.matrix(), 1e-10, 1e-14);
}

BOOST_DATA_TEST_CASE(RotationInZ, boost::unit_test::data::make(strategies),
                     strategy) {
  double hlZ = 400_mm;
  double gap = 100_mm;
  double shift = 300_mm;

  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 300_mm, hlZ);

  auto vol1 = std::make_shared<Volume>(
      Transform3::Identity() *
          Translation3{0_mm, 0_mm, -hlZ - gap / 2.0 + shift},
      bounds1);

  auto vol2 = std::make_shared<Volume>(
      Transform3::Identity() *
          Translation3{0_mm, 0_mm, hlZ + gap / 2.0 + shift} *
          AngleAxis3{30_degree, Vector3::UnitZ()},
      bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  CylinderVolumeStack cylStack(volumes, AxisDirection::AxisZ, strategy,
                               VolumeResizeStrategy::Gap, *logger);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR), 400_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    2 * hlZ + gap / 2.0);

  auto newBounds1 =
      dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
  auto newBounds2 =
      dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());

  for (const auto& bounds : {newBounds1, newBounds2}) {
    BOOST_CHECK_EQUAL(bounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(bounds->get(CylinderVolumeBounds::eMaxR), 400_mm);
  }

  if (strategy == VolumeAttachmentStrategy::Gap) {
    // Volumes stayed at the same position, not resized
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ - gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  } else if (strategy == VolumeAttachmentStrategy::First) {
    // Left volume moved, got resized
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 2.0);
    // Right volume stayed the same
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  } else if (strategy == VolumeAttachmentStrategy::Second) {
    // Left volume stayed the same
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ - gap / 2.0 + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
    // Right volume moved, got resized
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + shift);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 2.0);
  } else if (strategy == VolumeAttachmentStrategy::Midpoint) {
    // Left volume moved, got resized
    BOOST_CHECK_EQUAL(vol1->center()[eZ], -hlZ - gap / 4.0 + shift);
    BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 4.0);

    // Right volume moved, got resized
    BOOST_CHECK_EQUAL(vol2->center()[eZ], hlZ + gap / 4.0 + shift);
    BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                      hlZ + gap / 4.0);
  }
}

BOOST_DATA_TEST_CASE(UpdateStack,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(-100_mm, 0_mm, 100_mm) *
                      boost::unit_test::data::make(resizeStrategies)),
                     angle, offset, zshift, strategy) {
  double hlZ = 400_mm;

  // Cylinder volumes which already line up, but have different1 radii
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 600_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(100_mm, 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(100_mm, 600_mm, hlZ);

  Transform3 base = AngleAxis3(angle * 1_degree, Vector3::UnitX()) *
                    Translation3(offset + Vector3{0_mm, 0_mm, zshift});

  Transform3 transform1 = base;
  transform1.translate(Vector3{0_mm, 0_mm, -2 * hlZ});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  transform2.translate(Vector3{0_mm, 0_mm, 0_mm});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = base;
  transform3.translate(Vector3{0_mm, 0_mm, 2 * hlZ});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  std::vector<Volume*> originalVolumes = volumes;

  std::vector<Transform3> originalTransforms = {transform1, transform2,
                                                transform3};

  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisZ,
      VolumeAttachmentStrategy::Gap,  // should not make a
                                      // difference
      strategy, *logger);

  const auto* originalBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());

  auto assertOriginalBounds = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds, originalBounds);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      3 * hlZ);
  };

  assertOriginalBounds();

  {
    // Assign a copy of the identical bounds gives identical bounds
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    cylStack.update(bounds, std::nullopt, *logger);
    assertOriginalBounds();
  }

  {
    // Cannot increase mininmum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 200_mm);
    BOOST_CHECK_THROW(cylStack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 500_mm);
    BOOST_CHECK_THROW(cylStack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 2 * hlZ);
    BOOST_CHECK_THROW(cylStack.update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Reduce minimum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 50_mm);
    cylStack.update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    // Rest unchanged
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      3 * hlZ);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // All volumes reduces min r to accommodate
    for (const auto& [volume, origTransform] :
         zip(volumes, originalTransforms)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
    }
  }

  {
    // Increase maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 700_mm);
    cylStack.update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);
    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      3 * hlZ);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // All volumes reduces min r to accommodate
    for (const auto& [volume, origTransform] :
         zip(volumes, originalTransforms)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
    }
  }

  {
    // Increase half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 4 * hlZ);
    cylStack.update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      4 * hlZ);

    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);

    if (strategy == VolumeResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 got bigger and shifted left
      auto newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + hlZ / 2.0);
      Transform3 expectedTransform1 =
          base * Translation3{0_mm, 0_mm, -2 * hlZ - hlZ / 2.0};
      BOOST_CHECK_EQUAL(vol1->transform().matrix(),
                        expectedTransform1.matrix());

      // Volume 2 stayed the same
      auto newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), transform2.matrix());

      // Volume 3 got bigger and shifted right
      auto newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ + hlZ / 2.0);
      Transform3 expectedTransform3 =
          base * Translation3{0_mm, 0_mm, 2 * hlZ + hlZ / 2.0};
      BOOST_CHECK_EQUAL(vol3->transform().matrix(),
                        expectedTransform3.matrix());
    } else if (strategy == VolumeResizeStrategy::Gap) {
      // Gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);

      for (const auto& [volume, origTransform] :
           zip(originalVolumes, originalTransforms)) {
        const auto* newBounds =
            dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR), 700_mm);
        BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                          hlZ);
        // Position stayed the same
        BOOST_CHECK_EQUAL(volume->transform().matrix(), origTransform.matrix());
      }

      auto gap1 = volumes.front();
      auto gap2 = volumes.back();

      const auto* gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());

      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ / 2.0);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ / 2.0);

      Transform3 gap1Transform =
          base * Translation3{0_mm, 0_mm, -3 * hlZ - hlZ / 2.0};
      Transform3 gap2Transform =
          base * Translation3{0_mm, 0_mm, 3 * hlZ + hlZ / 2.0};

      CHECK_CLOSE_OR_SMALL(gap1->transform().matrix(), gap1Transform.matrix(),
                           1e-10, 1e-14);
      CHECK_CLOSE_OR_SMALL(gap2->transform().matrix(), gap2Transform.matrix(),
                           1e-10, 1e-14);
    }
  }
}

BOOST_DATA_TEST_CASE(
    UpdateStackOneSided,
    (boost::unit_test::data::make(-1.0, 1.0) ^
     boost::unit_test::data::make(VolumeResizeStrategy::Gap,
                                  VolumeResizeStrategy::Expand)),
    f, strategy) {
  auto trf = Transform3::Identity();

  auto trf1 = trf * Translation3{Vector3{0_mm, 0_mm, -500_mm}};
  auto vol1 = std::make_shared<Volume>(
      trf1, std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, 400_mm));

  auto trf2 = trf * Translation3{Vector3{0_mm, 0_mm, 500_mm}};
  auto vol2 = std::make_shared<Volume>(
      trf2, std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, 400_mm));

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  CylinderVolumeStack cylStack{volumes, AxisDirection::AxisZ,
                               VolumeAttachmentStrategy::Gap, strategy,
                               *logger};
  const auto* originalBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());

  // Increase halflength by 50mm
  auto newBounds = std::make_shared<CylinderVolumeBounds>(
      dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
  newBounds->set(CylinderVolumeBounds::eHalfLengthZ, 950_mm);
  // Shift to +z by 50mm
  trf *= Translation3{Vector3{0_mm, 0_mm, f * 50_mm}};
  // -> left edge should stay at -400mm, right edge should be at 500mm or the
  // other direction

  auto checkUnchanged = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(*cylBounds, *originalBounds);
  };

  // Invalid: shift too far in z
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 0, f * 20_mm}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in x
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{10_mm, 0, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in y
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 10_mm, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: rotation
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * AngleAxis3{10_degree, Vector3::UnitY()},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  cylStack.update(newBounds, trf, *logger);

  BOOST_CHECK_EQUAL(cylStack.transform().matrix(), trf.matrix());
  const auto* cylBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(cylBounds != nullptr);
  BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), 950_mm);

  // All volumes including gaps should have same r size
  for (const auto* vol : volumes) {
    const auto* volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMaxR), 300_mm);
  }

  if (strategy == VolumeResizeStrategy::Expand) {
    // No gaps were added, there was one gap initially
    BOOST_CHECK_EQUAL(volumes.size(), 3);
    const Volume* vol = nullptr;
    if (f < 0.0) {
      // first volume should have gotten bigger
      vol = volumes.front();
    } else {
      // last volume should have gotten bigger
      vol = volumes.back();
    }

    const auto* volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      450_mm);
    BOOST_CHECK_EQUAL(vol->center()[eZ], f * 550_mm);
  } else if (strategy == VolumeResizeStrategy::Gap) {
    // One gap volume was added
    BOOST_CHECK_EQUAL(volumes.size(), 4);

    const Volume* gap = nullptr;
    if (f < 0.0) {
      gap = volumes.front();
    } else {
      gap = volumes.back();
    }
    const auto* gapBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&gap->volumeBounds());
    BOOST_REQUIRE(gapBounds != nullptr);

    BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      50_mm);
    BOOST_CHECK_EQUAL(gap->center()[eZ], f * 950_mm);
  }
}

BOOST_AUTO_TEST_CASE(ResizeReproduction1) {
  Transform3 trf1 =
      Transform3::Identity() * Translation3{Vector3::UnitZ() * -2000};
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(70, 100, 100.0);
  Volume vol1{trf1, bounds1};

  std::vector<Volume*> volumes = {&vol1};
  CylinderVolumeStack stack(volumes, AxisDirection::AxisZ,
                            VolumeAttachmentStrategy::Gap,
                            VolumeResizeStrategy::Gap, *logger);

  Transform3 trf2 =
      Transform3::Identity() * Translation3{Vector3::UnitZ() * -1500};
  stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 600), trf2,
               *logger);

  std::cout << stack.volumeBounds() << std::endl;
  std::cout << stack.transform().matrix() << std::endl;

  Transform3 trf3 =
      Transform3::Identity() * Translation3{Vector3::UnitZ() * -1600};
  stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 700), trf3,
               *logger);
}

BOOST_AUTO_TEST_CASE(ResizeReproduction2) {
  // The numbers are tuned a bit to reproduce the faulty behavior
  Transform3 trf1 =
      Transform3::Identity() * Translation3{Vector3::UnitZ() * 263};
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(30, 100, 4.075);
  Volume vol1{trf1, bounds1};

  std::vector<Volume*> volumes = {&vol1};
  CylinderVolumeStack stack(volumes, AxisDirection::AxisZ,
                            VolumeAttachmentStrategy::Gap,
                            VolumeResizeStrategy::Gap, *logger);

  Transform3 trf2 =
      Transform3::Identity() * Translation3{Vector3::UnitZ() * 260.843};
  stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 6.232), trf2,
               *logger);

  std::cout << stack.volumeBounds() << std::endl;
  std::cout << stack.transform().matrix() << std::endl;

  Transform3 trf3 =
      Transform3::Identity() * Translation3{Vector3::UnitZ() * 1627.31};
  stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 1372.699),
               trf3, *logger);
}

//   original size
// <--------------->
// +---------------+
// |               |
// |               |
// |   Volume 1    |
// |               |
// |               |
// +---------------+
//         first resize
// <-------------------------->
// +---------------+----------+
// |               |          |
// |               |          |
// |   Volume 1    |   Gap    |
// |               |          |      Gap is
// |               |          |      reused!--+
// +---------------+----------+               |
//             second resize                  |
// <----------------------------------->      |
// +---------------+-------------------+      |
// |               |                   |      |
// |               |                   |      |
// |   Volume 1    |        Gap        |<-----+
// |               |                   |
// |               |                   |
// +---------------+-------------------+
//
BOOST_AUTO_TEST_CASE(ResizeGapMultiple) {
  Transform3 trf = Transform3::Identity();
  auto bounds = std::make_shared<CylinderVolumeBounds>(70, 100, 100.0);
  Volume vol{trf, bounds};

  BOOST_TEST_CONTEXT("Positive") {
    std::vector<Volume*> volumes = {&vol};
    CylinderVolumeStack stack(volumes, AxisDirection::AxisZ,
                              VolumeAttachmentStrategy::Gap,
                              VolumeResizeStrategy::Gap, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 1);
    BOOST_CHECK(stack.gaps().empty());

    stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 200),
                 trf * Translation3{Vector3::UnitZ() * 100}, *logger);
    BOOST_CHECK_EQUAL(volumes.size(), 2);
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[eZ], 200.0);
    const auto* cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      100.0);

    stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 300),
                 trf * Translation3{Vector3::UnitZ() * 200}, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 2);
    // No additional gap volume was added!
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[eZ], 300.0);
    cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      200.0);
  }

  BOOST_TEST_CONTEXT("Negative") {
    std::vector<Volume*> volumes = {&vol};
    CylinderVolumeStack stack(volumes, AxisDirection::AxisZ,
                              VolumeAttachmentStrategy::Gap,
                              VolumeResizeStrategy::Gap, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 1);
    BOOST_CHECK(stack.gaps().empty());

    stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 200),
                 trf * Translation3{Vector3::UnitZ() * -100}, *logger);
    BOOST_CHECK_EQUAL(volumes.size(), 2);
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[eZ], -200.0);
    const auto* cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      100.0);

    stack.update(std::make_shared<CylinderVolumeBounds>(30.0, 100, 300),
                 trf * Translation3{Vector3::UnitZ() * -200}, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 2);
    // No additional gap volume was added!
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    BOOST_CHECK_EQUAL(stack.gaps().front()->center()[eZ], -300.0);
    cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      200.0);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(RDirection)

BOOST_DATA_TEST_CASE(Baseline,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::xrange(0, 2, 1) *
                      boost::unit_test::data::make(-0.1, 0.0, 0.1) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(strategies)),
                     angle, rotate, f, offset, strategy) {
  double hlZ = 400_mm;

  double fInner = 1.0 + f;
  double fOuter = 1.0 - f;

  // Cylinder volumes which already line up in r but have different z and hl
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(fInner * 100_mm,
                                                        fOuter * 300_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(fInner * 300_mm,
                                                        fOuter * 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(fInner * 600_mm,
                                                        fOuter * 900_mm, hlZ);

  Transform3 base =
      AngleAxis3(angle * 1_degree, Vector3::UnitX()) * Translation3(offset);

  // volumes are shifted in z

  Transform3 transform1 = base;
  transform1.translate(Vector3{0_mm, 0_mm, 20_mm});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = base;
  transform2.translate(Vector3{0_mm, 0_mm, -30_mm});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = base;
  transform3.translate(Vector3{0_mm, 0_mm, 40_mm});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Rotate to simulate unsorted volumes: all results should be the same!
  std::rotate(volumes.begin(), volumes.begin() + rotate, volumes.end());

  std::vector<Volume*> origVolumes = volumes;

  std::vector<CylinderVolumeBounds> originalBounds;
  std::transform(
      volumes.begin(), volumes.end(), std::back_inserter(originalBounds),
      [](const auto& vol) {
        return dynamic_cast<const CylinderVolumeBounds&>(vol->volumeBounds());
      });

  if (f < 0.0) {
    BOOST_CHECK_THROW(
        CylinderVolumeStack(volumes, AxisDirection::AxisR, strategy,
                            VolumeResizeStrategy::Gap, *logger),
        std::invalid_argument);
    return;
  }

  CylinderVolumeStack cylStack(volumes, AxisDirection::AxisR, strategy,
                               VolumeResizeStrategy::Gap, *logger);

  auto stackBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(stackBounds != nullptr);

  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMinR),
                    fInner * 100_mm);
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eMaxR),
                    fOuter * 900_mm);
  double expectedHalfLengthZ = (40_mm + 30_mm + 2 * hlZ) / 2.0;
  BOOST_CHECK_EQUAL(stackBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    expectedHalfLengthZ);

  // After synchronization, all volumes should have the same z position and half
  // length
  // This includes possible gap volumes!
  Transform3 commonTransform = base * Translation3{0_mm, 0_mm, 5_mm};

  CHECK_CLOSE_OR_SMALL(cylStack.transform().matrix(), commonTransform.matrix(),
                       1e-10, 1e-14);

  for (const auto& volume : volumes) {
    const auto* cylinderBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
    BOOST_REQUIRE(cylinderBounds != nullptr);
    BOOST_CHECK_EQUAL(cylinderBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      expectedHalfLengthZ);
  }

  BOOST_CHECK_EQUAL(
      dynamic_cast<const CylinderVolumeBounds&>(vol1->volumeBounds())
          .get(CylinderVolumeBounds::eMinR),
      fInner * 100_mm);

  BOOST_CHECK_EQUAL(
      dynamic_cast<const CylinderVolumeBounds&>(vol3->volumeBounds())
          .get(CylinderVolumeBounds::eMaxR),
      fOuter * 900_mm);

  // Volumes are sorted in r
  for (std::size_t i = 0; i < volumes.size() - 1; ++i) {
    const auto& a = volumes.at(i);
    const auto& b = volumes.at(i + 1);

    const auto* aBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&a->volumeBounds());
    const auto* bBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&b->volumeBounds());

    double aMidR = (aBounds->get(CylinderVolumeBounds::eMinR) +
                    aBounds->get(CylinderVolumeBounds::eMaxR)) /
                   2.0;

    double bMidR = (bBounds->get(CylinderVolumeBounds::eMinR) +
                    bBounds->get(CylinderVolumeBounds::eMaxR)) /
                   2.0;

    BOOST_CHECK_LT(aMidR, bMidR);
  }

  if (f == 0.0) {
    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    // Original volumes did not change r bounds
    for (const auto& [volume, origCylBounds] :
         zip(origVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR),
                        origCylBounds.get(CylinderVolumeBounds::eMinR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR),
                        origCylBounds.get(CylinderVolumeBounds::eMaxR));
    }
  } else {
    const auto* newBounds1 =
        dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
    const auto* newBounds2 =
        dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
    const auto* newBounds3 =
        dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
    if (strategy == VolumeAttachmentStrategy::Gap) {
      // Two gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 5);

      // Original volumes did not change r bounds
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        fInner * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        fInner * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);

      auto gap1 = volumes.at(1);
      auto gap2 = volumes.at(3);

      const auto* gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      const auto* gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());

      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMinR),
                        fOuter * 300_mm);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMaxR),
                        fInner * 300_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMinR),
                        fOuter * 600_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMaxR),
                        fInner * 600_mm);

    } else if (strategy == VolumeAttachmentStrategy::First) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 got bigger and grew to meet Volume 2
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        fInner * 300_mm);

      // Volume 2 got bigger and grew to meet Volume 3
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        fInner * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        fInner * 600_mm);

      // Volume 3 stayed the same
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        fInner * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);

    } else if (strategy == VolumeAttachmentStrategy::Second) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 stayed the same
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 300_mm);

      // Volume 2 got bigger and grew inward to meet Volume 1
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        fOuter * 300_mm);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 600_mm);

      // Volume 3 got bigger and grew inward to meet Volume 2
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        fOuter * 600_mm);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);
    } else if (strategy == VolumeAttachmentStrategy::Midpoint) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Volume 1 grew outward to meet Volume 2 half way
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR),
                        fInner * 100_mm);
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMaxR),
                        (fOuter * 300_mm + fInner * 300_mm) / 2.0);

      // Volume 2 grew inward and outward to meet Volume 1 and 3 half way
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMinR),
                        (fOuter * 300_mm + fInner * 300_mm) / 2.0);
      BOOST_CHECK_EQUAL(newBounds2->get(CylinderVolumeBounds::eMaxR),
                        (fOuter * 600_mm + fInner * 600_mm) / 2.0);

      // Volume 3 grew inward to meet Volume 2 half way
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMinR),
                        (fOuter * 600_mm + fInner * 600_mm) / 2.0);
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR),
                        fOuter * 900_mm);
    }
  }
}

BOOST_DATA_TEST_CASE(UpdateStack,
                     (boost::unit_test::data::xrange(-135, 180, 45) *
                      boost::unit_test::data::make(Vector3{0_mm, 0_mm, 0_mm},
                                                   Vector3{20_mm, 0_mm, 0_mm},
                                                   Vector3{0_mm, 20_mm, 0_mm},
                                                   Vector3{20_mm, 20_mm, 0_mm},
                                                   Vector3{0_mm, 0_mm, 20_mm}) *
                      boost::unit_test::data::make(-100_mm, 0_mm, 100_mm) *
                      boost::unit_test::data::make(resizeStrategies)),
                     angle, offset, zshift, strategy) {
  double hlZ = 400_mm;

  // Cylinder volumes which already line up in r but have different z and hl
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(300_mm, 600_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(600_mm, 900_mm, hlZ);

  Transform3 base = AngleAxis3(angle * 1_degree, Vector3::UnitX()) *
                    Translation3(offset + Vector3{0, 0, zshift});

  // volumes are shifted in z
  auto vol1 = std::make_shared<Volume>(base, bounds1);
  auto vol2 = std::make_shared<Volume>(base, bounds2);
  auto vol3 = std::make_shared<Volume>(base, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  std::vector<Volume*> originalVolumes = volumes;

  std::vector<CylinderVolumeBounds> originalBounds;

  std::transform(
      volumes.begin(), volumes.end(), std::back_inserter(originalBounds),
      [](const auto& vol) {
        return *dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
      });

  const CylinderVolumeBounds* originalOuterBounds = nullptr;

  std::unique_ptr<CylinderVolumeStack> cylStack;

  auto resetCylStack = [&]() {
    volumes = originalVolumes;

    for (const auto& [volume, origBounds] : zip(volumes, originalBounds)) {
      volume->assignVolumeBounds(
          std::make_shared<CylinderVolumeBounds>(origBounds));
    }

    cylStack = std::make_unique<CylinderVolumeStack>(
        volumes, AxisDirection::AxisR,
        VolumeAttachmentStrategy::Gap,  // should not make a
                                        // difference
        strategy, *logger);

    originalOuterBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
  };

  resetCylStack();

  auto assertInitialVolumesUnchanged = [&]() {
    for (const auto& [volume, origCylBounds] :
         zip(originalVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR),
                        origCylBounds.get(CylinderVolumeBounds::eMinR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR),
                        origCylBounds.get(CylinderVolumeBounds::eMaxR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        origCylBounds.get(CylinderVolumeBounds::eHalfLengthZ));
      BOOST_CHECK_EQUAL(volume->transform().matrix(), base.matrix());
    }
  };

  auto assertOriginalBounds = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds, originalOuterBounds);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 900_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  };

  assertOriginalBounds();

  {
    // Assign a copy of the identical bounds gives identical bounds
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    cylStack->update(bounds, std::nullopt, *logger);
    assertOriginalBounds();
  }

  {
    // Cannot increase mininmum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 200_mm);
    BOOST_CHECK_THROW(cylStack->update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 500_mm);
    BOOST_CHECK_THROW(cylStack->update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Cannot decrease half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 0.5 * hlZ);
    BOOST_CHECK_THROW(cylStack->update(bounds, std::nullopt, *logger),
                      std::invalid_argument);
    assertOriginalBounds();
  }

  {
    // Reduce minimum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMinR, 50_mm);
    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
    // Rest unchanged
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 900_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

    if (strategy == VolumeResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Innermost volume reduced r size
      const auto* newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR), 50_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      const auto* newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds2, originalBounds[1]);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), base.matrix());

      const auto* newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds3, originalBounds[2]);
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), base.matrix());

    } else if (strategy == VolumeResizeStrategy::Gap) {
      // One gap volume was added
      BOOST_CHECK_EQUAL(volumes.size(), 4);

      auto gap = volumes.front();
      auto gapBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&gap->volumeBounds());
      BOOST_REQUIRE(gapBounds != nullptr);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), 100_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(gap->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      assertInitialVolumesUnchanged();
    }
  }

  resetCylStack();

  {
    // Increase maximum r
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eMaxR, 1000_mm);
    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 1000_mm);
    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

    if (strategy == VolumeResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Outermost volume increased r size
      const auto* newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR), 1000_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      const auto* newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds1, originalBounds[0]);
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), base.matrix());

      const auto* newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds2, originalBounds[1]);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), base.matrix());

    } else if (strategy == VolumeResizeStrategy::Gap) {
      // One gap volume was added
      BOOST_CHECK_EQUAL(volumes.size(), 4);

      auto gap = volumes.back();
      auto gapBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&gap->volumeBounds());
      BOOST_REQUIRE(gapBounds != nullptr);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), 900_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), 1000_mm);
      BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(gap->transform().matrix(), base.matrix());

      // Other volumes are unchanged
      assertInitialVolumesUnchanged();
    }
  }

  resetCylStack();

  {
    // Decrease r min and increase r max
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set({
        {CylinderVolumeBounds::eMinR, 0_mm},
        {CylinderVolumeBounds::eMaxR, 1100_mm},
    });

    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 1100_mm);
    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 0_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

    if (strategy == VolumeResizeStrategy::Expand) {
      // No gap volumes were added
      BOOST_CHECK_EQUAL(volumes.size(), 3);

      // Innermost volume reduced r size
      const auto* newBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol1->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds1->get(CylinderVolumeBounds::eMinR), 0_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol1->transform().matrix(), base.matrix());

      // Middle volume is unchanged
      const auto* newBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol2->volumeBounds());
      BOOST_CHECK_EQUAL(*newBounds2, originalBounds[1]);
      BOOST_CHECK_EQUAL(vol2->transform().matrix(), base.matrix());

      // Outermost volume increased r size
      const auto* newBounds3 =
          dynamic_cast<const CylinderVolumeBounds*>(&vol3->volumeBounds());
      BOOST_CHECK_EQUAL(newBounds3->get(CylinderVolumeBounds::eMaxR), 1100_mm);
      // Position stayed the same
      BOOST_CHECK_EQUAL(vol3->transform().matrix(), base.matrix());

    } else if (strategy == VolumeResizeStrategy::Gap) {
      // One gap volume was added
      BOOST_CHECK_EQUAL(volumes.size(), 5);

      auto gap1 = volumes.front();
      auto gapBounds1 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap1->volumeBounds());
      BOOST_REQUIRE(gapBounds1 != nullptr);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMinR), 0_mm);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eMaxR), 100_mm);
      BOOST_CHECK_EQUAL(gapBounds1->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);
      BOOST_CHECK_EQUAL(gap1->transform().matrix(), base.matrix());

      auto gap2 = volumes.back();
      auto gapBounds2 =
          dynamic_cast<const CylinderVolumeBounds*>(&gap2->volumeBounds());
      BOOST_REQUIRE(gapBounds2 != nullptr);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMinR), 900_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eMaxR), 1100_mm);
      BOOST_CHECK_EQUAL(gapBounds2->get(CylinderVolumeBounds::eHalfLengthZ),
                        hlZ);

      // Other volumes are unchanged
      assertInitialVolumesUnchanged();
    }
  }

  resetCylStack();

  {
    // Increase half length z
    auto bounds = std::make_shared<CylinderVolumeBounds>(
        dynamic_cast<const CylinderVolumeBounds&>(cylStack->volumeBounds()));
    bounds->set(CylinderVolumeBounds::eHalfLengthZ, 2 * hlZ);
    cylStack->update(bounds, std::nullopt, *logger);
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack->volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      2 * hlZ);

    // Rest as before
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 900_mm);

    // No gap volumes were added
    BOOST_CHECK_EQUAL(volumes.size(), 3);

    for (const auto& [volume, origCylBounds] :
         zip(originalVolumes, originalBounds)) {
      const auto* newBounds =
          dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
      // Radii are all as before
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMinR),
                        origCylBounds.get(CylinderVolumeBounds::eMinR));
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eMaxR),
                        origCylBounds.get(CylinderVolumeBounds::eMaxR));

      // Half length z is changed on all
      BOOST_CHECK_EQUAL(newBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                        2 * hlZ);

      // Position stayed the same
      BOOST_CHECK_EQUAL(volume->transform().matrix(), base.matrix());
    }
  }
}

BOOST_DATA_TEST_CASE(
    UpdateStackOneSided,
    (boost::unit_test::data::make(-1.0, 1.0) ^
     boost::unit_test::data::make(VolumeResizeStrategy::Gap,
                                  VolumeResizeStrategy::Expand)),
    f, strategy) {
  // Strategy should not affect the sizing here at all

  auto trf = Transform3::Identity();

  auto vol1 = std::make_shared<Volume>(
      trf, std::make_shared<CylinderVolumeBounds>(100_mm, 300_mm, 400_mm));

  auto vol2 = std::make_shared<Volume>(
      trf, std::make_shared<CylinderVolumeBounds>(400_mm, 600_mm, 400_mm));

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  CylinderVolumeStack cylStack{volumes, AxisDirection::AxisR,
                               VolumeAttachmentStrategy::Gap, strategy,
                               *logger};
  const auto* originalBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());

  // Increase halflength by 50mm
  auto newBounds = std::make_shared<CylinderVolumeBounds>(
      dynamic_cast<const CylinderVolumeBounds&>(cylStack.volumeBounds()));
  newBounds->set(CylinderVolumeBounds::eHalfLengthZ, 450_mm);
  // Shift to +z by 50mm
  trf *= Translation3{Vector3{0_mm, 0_mm, f * 50_mm}};
  // -> left edge should stay at -400mm, right edge should be at 500mm

  auto checkUnchanged = [&]() {
    const auto* cylBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
    BOOST_REQUIRE(cylBounds != nullptr);
    BOOST_CHECK_EQUAL(*cylBounds, *originalBounds);
  };

  // Invalid: shift too far in z
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 0, f * 20_mm}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in x
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{10_mm, 0, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: shift in y
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * Translation3{Vector3{0, 10_mm, 0}},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  // Invalid: rotation
  BOOST_CHECK_THROW(
      cylStack.update(newBounds, trf * AngleAxis3{10_degree, Vector3::UnitY()},
                      *logger),
      std::invalid_argument);
  checkUnchanged();

  cylStack.update(newBounds, trf, *logger);

  BOOST_CHECK_EQUAL(cylStack.transform().matrix(), trf.matrix());
  const auto* cylBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&cylStack.volumeBounds());
  BOOST_REQUIRE(cylBounds != nullptr);
  BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 100_mm);
  BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 600_mm);

  // All volumes including gaps should have same new position and halflength
  for (const auto* vol : volumes) {
    const auto* volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&vol->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(vol->transform().matrix(), trf.matrix());
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                      450_mm);
  }
}

BOOST_AUTO_TEST_CASE(ResizeGapMultiple) {
  Transform3 trf = Transform3::Identity();
  auto bounds = std::make_shared<CylinderVolumeBounds>(100, 200, 100);
  Volume vol{trf, bounds};

  BOOST_TEST_CONTEXT("Outer") {
    std::vector<Volume*> volumes = {&vol};
    CylinderVolumeStack stack(volumes, AxisDirection::AxisR,
                              VolumeAttachmentStrategy::Gap,
                              VolumeResizeStrategy::Gap, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 1);
    BOOST_CHECK(stack.gaps().empty());

    stack.update(std::make_shared<CylinderVolumeBounds>(100, 250, 100), trf,
                 *logger);
    BOOST_CHECK_EQUAL(volumes.size(), 2);
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    const auto* cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 200);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 250);

    stack.update(std::make_shared<CylinderVolumeBounds>(100, 300, 100), trf,
                 *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 2);
    // No additional gap volume was added!
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 200);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 300);
  }

  BOOST_TEST_CONTEXT("Inner") {
    std::vector<Volume*> volumes = {&vol};
    CylinderVolumeStack stack(volumes, AxisDirection::AxisR,
                              VolumeAttachmentStrategy::Gap,
                              VolumeResizeStrategy::Gap, *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 1);
    BOOST_CHECK(stack.gaps().empty());

    stack.update(std::make_shared<CylinderVolumeBounds>(50, 200, 100), trf,
                 *logger);
    BOOST_CHECK_EQUAL(volumes.size(), 2);
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    const auto* cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 50);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 100);

    stack.update(std::make_shared<CylinderVolumeBounds>(0, 200, 100), trf,
                 *logger);

    BOOST_CHECK_EQUAL(volumes.size(), 2);
    // No additional gap volume was added!
    BOOST_CHECK_EQUAL(stack.gaps().size(), 1);

    cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &stack.gaps().front()->volumeBounds());
    BOOST_REQUIRE_NE(cylBounds, nullptr);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMinR), 0);
    BOOST_CHECK_EQUAL(cylBounds->get(CylinderVolumeBounds::eMaxR), 100);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Common)

BOOST_DATA_TEST_CASE(JoinCylinderVolumesInvalidDirection,
                     boost::unit_test::data::make(strategies), strategy) {
  std::vector<Volume*> volumes;
  auto vol1 = std::make_shared<Volume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
  volumes.push_back(vol1.get());

  // Single volume invalid direction still gives an error
  BOOST_CHECK_THROW(
      CylinderVolumeStack(volumes, AxisDirection::AxisY, strategy),
      std::invalid_argument);

  auto vol2 = std::make_shared<Volume>(
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
  volumes.push_back(vol2.get());

  BOOST_CHECK_THROW(
      CylinderVolumeStack(volumes, AxisDirection::AxisY, strategy),
      std::invalid_argument);
}

BOOST_DATA_TEST_CASE(JoinCylinderVolumesInvalidInput,
                     (boost::unit_test::data::make(strategies) *
                      boost::unit_test::data::make(Acts::AxisDirection::AxisZ,
                                                   Acts::AxisDirection::AxisR)),
                     strategy, direction) {
  BOOST_TEST_CONTEXT("Empty Volume") {
    std::vector<Volume*> volumes;
    BOOST_CHECK_THROW(CylinderVolumeStack(volumes, direction, strategy),
                      std::invalid_argument);
  }

  BOOST_TEST_CONTEXT("Volumes rotated relative to each other") {
    // At this time, all rotations are considered invalid, even around z
    for (const Vector3 axis : {Vector3::UnitX(), Vector3::UnitY()}) {
      std::vector<Volume*> volumes;
      auto vol1 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol1.get());

      BOOST_TEST_MESSAGE("Axis: " << axis);
      auto vol2 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm}} *
                     AngleAxis3(1_degree, axis)},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol2.get());

      BOOST_CHECK_THROW(CylinderVolumeStack(volumes, direction, strategy,
                                            VolumeResizeStrategy::Gap, *logger),
                        std::invalid_argument);
    }
  }

  BOOST_TEST_CONTEXT("Volumes shifted in the xy plane relative to each other") {
    for (const Vector3& shift :
         {Vector3{5_mm, 0, 0}, Vector3{0, -5_mm, 0}, Vector3{2_mm, -2_mm, 0}}) {
      std::vector<Volume*> volumes;
      auto vol1 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol1.get());

      auto vol2 = std::make_shared<Volume>(
          Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm} + shift}},
          std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
      volumes.push_back(vol2.get());

      BOOST_CHECK_THROW(CylinderVolumeStack(volumes, direction, strategy,
                                            VolumeResizeStrategy::Gap, *logger),
                        std::invalid_argument);
    }
  }

  BOOST_TEST_CONTEXT("Volume has phi values or bevel values") {
    std::vector<std::shared_ptr<CylinderVolumeBounds>> invalidVolumeBounds = {
        std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm,
                                               0.2 * std::numbers::pi),

        std::make_shared<CylinderVolumeBounds>(
            100_mm, 400_mm, 400_mm, std::numbers::pi, 0.3 * std::numbers::pi),

        std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm,
                                               std::numbers::pi, 0.,
                                               0.3 * std::numbers::pi),
        std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm,
                                               std::numbers::pi, 0., 0.,
                                               0.3 * std::numbers::pi),
    };

    for (const auto& invalid : invalidVolumeBounds) {
      std::stringstream ss;
      ss << "Invalid bounds: " << *invalid;
      BOOST_TEST_CONTEXT(ss.str()) {
        std::vector<Volume*> volumes;
        auto vol1 = std::make_shared<Volume>(
            Transform3{Translation3{Vector3{0_mm, 0_mm, -500_mm}}},
            std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));
        volumes.push_back(vol1.get());

        {
          // have valid stack, try to assign extra
          CylinderVolumeStack cylStack(volumes, direction, strategy,
                                       VolumeResizeStrategy::Gap, *logger);
          BOOST_CHECK_THROW(cylStack.update(invalid, std::nullopt, *logger),
                            std::invalid_argument);
        }

        {
          std::shared_ptr<Volume> vol;
          if (direction == AxisDirection::AxisZ) {
            vol = std::make_shared<Volume>(
                Transform3{Translation3{Vector3{0_mm, 0_mm, 500_mm}}}, invalid);
          } else {
            invalid->set({
                {CylinderVolumeBounds::eMinR, 400_mm},
                {CylinderVolumeBounds::eMaxR, 600_mm},
            });
            vol = std::make_shared<Volume>(
                Transform3{Translation3{Vector3{0_mm, 0_mm, 0_mm}}}, invalid);
          }
          volumes.push_back(vol.get());
          BOOST_CHECK_THROW(
              CylinderVolumeStack(volumes, direction, strategy,
                                  VolumeResizeStrategy::Gap, *logger),
              std::invalid_argument);
        }
      }
    }
  }
}

BOOST_DATA_TEST_CASE(JoinCylinderVolumeSingle,
                     (boost::unit_test::data::make(Acts::AxisDirection::AxisZ,
                                                   Acts::AxisDirection::AxisR) *
                      boost::unit_test::data::make(strategies)),
                     direction, strategy) {
  auto vol = std::make_shared<Volume>(
      Transform3::Identity() * Translation3{14_mm, 24_mm, 0_mm} *
          AngleAxis3(73_degree, Vector3::UnitX()),
      std::make_shared<CylinderVolumeBounds>(100_mm, 400_mm, 400_mm));

  std::vector<Volume*> volumes{vol.get()};

  CylinderVolumeStack cylStack(volumes, direction, strategy,
                               VolumeResizeStrategy::Gap, *logger);

  // Cylinder stack has the same transform as bounds as the single input
  // volume
  BOOST_CHECK_EQUAL(volumes.size(), 1);
  BOOST_CHECK_EQUAL(volumes.at(0), vol.get());
  BOOST_CHECK_EQUAL(vol->transform().matrix(), cylStack.transform().matrix());
  BOOST_CHECK_EQUAL(vol->volumeBounds(), cylStack.volumeBounds());
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE(AsymmetricResizeZ) {
  double hlZ = 400_mm;
  double rMin = 100_mm;
  double rMax = 200_mm;

  // Create three cylinder volumes stacked in z
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);

  Transform3 transform1 = Transform3::Identity();
  transform1.translate(Vector3{0_mm, 0_mm, -2 * hlZ});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = Transform3::Identity();
  transform2.translate(Vector3{0_mm, 0_mm, 0_mm});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  Transform3 transform3 = Transform3::Identity();
  transform3.translate(Vector3{0_mm, 0_mm, 2 * hlZ});
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Test with Gap for negative z and Expand for positive z
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisZ, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Gap, VolumeResizeStrategy::Expand}, *logger);
  // Initial stack spans [-3*hlZ, 3*hlZ]. Update bounds to test asymmetric
  // resize in z only New bounds should span [-4*hlZ, 4*hlZ] to ensure we only
  // grow
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin, rMax, 4 * hlZ);
  Transform3 newTransform =
      Transform3::Identity() * Translation3{0_mm, 0_mm, 0_mm};

  cylStack.update(newBounds, newTransform, *logger);

  // Check that we have one gap volume at negative z
  BOOST_CHECK_EQUAL(volumes.size(), 4);  // Original 3 + 1 gap volume

  // Check gap volume at negative z
  auto gapVol = volumes.front();
  auto gapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&gapVol->volumeBounds());
  BOOST_REQUIRE(gapBounds != nullptr);
  BOOST_CHECK_EQUAL(
      gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
      hlZ / 2);  // Half the original half-length to fill 1*hlZ gap
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(gapVol->center()[eZ], -3.5 * hlZ,
                    1e-10);  // Center of [-4*hlZ, -3*hlZ]

  // Check that last volume was expanded in positive z
  auto* lastVol = volumes.back();
  BOOST_CHECK_EQUAL(lastVol, vol3.get());
  auto lastBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&lastVol->volumeBounds());
  BOOST_REQUIRE(lastBounds != nullptr);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    1.5 * hlZ);  // Original hlZ plus 0.5*hlZ expansion
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(lastVol->center()[eZ], 2.5 * hlZ,
                    1e-10);  // Center of [2*hlZ, 3*hlZ]

  // Check middle volumes maintain their size
  for (std::size_t i = 1; i < volumes.size() - 1; i++) {
    auto volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volumes[i]->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMinR), rMin);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  }
  BOOST_CHECK_CLOSE(volumes[1]->center()[eZ], -2 * hlZ, 1e-10);
  BOOST_CHECK_CLOSE(volumes[2]->center()[eZ], 0, 1e-10);
}

BOOST_AUTO_TEST_CASE(AsymmetricResizeZFlipped) {
  double hlZ = 400_mm;
  double rMin = 100_mm;
  double rMax = 200_mm;

  // Create three cylinder volumes stacked in z
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);

  Transform3 transform1 = Transform3::Identity() * Translation3(0, 0, -2 * hlZ);
  Transform3 transform2 = Transform3::Identity();
  Transform3 transform3 = Transform3::Identity() * Translation3(0, 0, 2 * hlZ);

  auto vol1 = std::make_shared<Volume>(transform1, bounds1);
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);
  auto vol3 = std::make_shared<Volume>(transform3, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Test with Expand for inner radius and Gap for outer radius
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisZ, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Expand, VolumeResizeStrategy::Gap}, *logger);

  // Update bounds to test asymmetric expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin, rMax, 4 * hlZ);
  cylStack.update(newBounds, std::nullopt, *logger);
  // Check that we have one gap volume at positive z
  BOOST_CHECK_EQUAL(volumes.size(), 4);  // Original 3 + 1 gap volume

  // Check gap volume at positive z
  auto gapVol = volumes.back();
  auto gapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&gapVol->volumeBounds());
  BOOST_REQUIRE(gapBounds != nullptr);
  BOOST_CHECK_EQUAL(
      gapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
      hlZ / 2);  // Half the original half-length to fill 1*hlZ gap
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(gapVol->center()[eZ], 3.5 * hlZ,
                    1e-10);  // Center of [3*hlZ, 4*hlZ]

  // Check that first volume was expanded in positive z
  auto* firstVol = volumes.front();
  BOOST_CHECK_EQUAL(firstVol, vol1.get());
  auto firstBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&firstVol->volumeBounds());
  BOOST_REQUIRE(firstBounds != nullptr);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    1.5 * hlZ);  // Original hlZ plus 0.5*hlZ expansion
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(firstVol->center()[eZ], -2.5 * hlZ,
                    1e-10);  // Center of [-3*hlZ, -2*hlZ]

  // Check middle volumes maintain their size
  for (std::size_t i = 1; i < volumes.size() - 1; i++) {
    auto volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volumes[i]->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMinR), rMin);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  }
  BOOST_CHECK_CLOSE(volumes[1]->center()[eZ], 0, 1e-10);
  BOOST_CHECK_CLOSE(volumes[2]->center()[eZ], 2 * hlZ, 1e-10);
}

BOOST_AUTO_TEST_CASE(AsymmetricResizeR) {
  double hlZ = 400_mm;

  // Create three cylinder volumes stacked in r with gaps
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 200_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 300_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(300_mm, 400_mm, hlZ);

  Transform3 transform = Transform3::Identity();
  auto vol1 = std::make_shared<Volume>(transform, bounds1);
  auto vol2 = std::make_shared<Volume>(transform, bounds2);
  auto vol3 = std::make_shared<Volume>(transform, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Test with Gap for inner radius and Expand for outer radius
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisR, VolumeAttachmentStrategy::Midpoint,
      {VolumeResizeStrategy::Gap, VolumeResizeStrategy::Expand}, *logger);

  // Update bounds to test asymmetric resize in r only
  auto newBounds = std::make_shared<CylinderVolumeBounds>(50_mm, 500_mm, hlZ);
  cylStack.update(newBounds, std::nullopt, *logger);
  // Check that we have one gap volume at inner radius
  BOOST_CHECK_EQUAL(volumes.size(), 4);  // Original 3 + 1 gap volume

  // Check gap volume at inner radius
  auto innerGap = volumes.front();
  auto innerGapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&innerGap->volumeBounds());
  BOOST_REQUIRE(innerGapBounds != nullptr);
  BOOST_CHECK_EQUAL(innerGapBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
  BOOST_CHECK_EQUAL(innerGapBounds->get(CylinderVolumeBounds::eMaxR), 100_mm);
  BOOST_CHECK_EQUAL(innerGapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    hlZ);

  // Check that outer volume was expanded
  auto* outerVol = volumes.back();
  BOOST_CHECK_EQUAL(outerVol, vol3.get());

  auto outerBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&outerVol->volumeBounds());
  BOOST_REQUIRE(outerBounds != nullptr);
  BOOST_CHECK_EQUAL(outerBounds->get(CylinderVolumeBounds::eMinR), 300_mm);
  BOOST_CHECK_EQUAL(outerBounds->get(CylinderVolumeBounds::eMaxR), 500_mm);
  BOOST_CHECK_EQUAL(outerBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

  // Check middle volumes maintain their size
  for (std::size_t i = 1; i < volumes.size() - 1; i++) {
    auto volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volumes[i]->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  }
}

BOOST_AUTO_TEST_CASE(AsymmetricResizeRFlipped) {
  double hlZ = 400_mm;

  // Create three cylinder volumes stacked in r
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(100_mm, 200_mm, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(200_mm, 300_mm, hlZ);
  auto bounds3 = std::make_shared<CylinderVolumeBounds>(300_mm, 400_mm, hlZ);

  Transform3 transform = Transform3::Identity();
  auto vol1 = std::make_shared<Volume>(transform, bounds1);
  auto vol2 = std::make_shared<Volume>(transform, bounds2);
  auto vol3 = std::make_shared<Volume>(transform, bounds3);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get(), vol3.get()};
  // Test with Expand for inner radius and Gap for outer radius
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisR, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Expand, VolumeResizeStrategy::Gap}, *logger);

  // Update bounds to test asymmetric expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(50_mm, 500_mm, hlZ);
  cylStack.update(newBounds, std::nullopt, *logger);
  // Check that we have one gap volume at outer radius
  BOOST_CHECK_EQUAL(volumes.size(), 4);  // Original 3 + 1 gap volume

  // Check gap volume at outer radius
  auto outerGap = volumes.back();
  auto outerGapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&outerGap->volumeBounds());
  BOOST_REQUIRE(outerGapBounds != nullptr);
  BOOST_CHECK_EQUAL(outerGapBounds->get(CylinderVolumeBounds::eMinR), 400_mm);
  BOOST_CHECK_EQUAL(outerGapBounds->get(CylinderVolumeBounds::eMaxR), 500_mm);
  BOOST_CHECK_EQUAL(outerGapBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    hlZ);

  // Check that inner volume was expanded
  auto* innerVol = volumes.front();
  BOOST_CHECK_EQUAL(innerVol, vol1.get());

  auto innerBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&innerVol->volumeBounds());
  BOOST_REQUIRE(innerBounds != nullptr);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eMaxR), 200_mm);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

  // Check middle volumes maintain their size
  for (std::size_t i = 1; i < volumes.size() - 1; i++) {
    auto volBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volumes[i]->volumeBounds());
    BOOST_REQUIRE(volBounds != nullptr);
    BOOST_CHECK_EQUAL(volBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  }
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeZ) {
  double hlZ = 400_mm;
  double rMin = 100_mm;
  double rMax = 200_mm;

  // Create two cylinder volumes stacked in z
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);

  Transform3 transform1 = Transform3::Identity();
  transform1.translate(Vector3{0_mm, 0_mm, -hlZ});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = Transform3::Identity();
  transform2.translate(Vector3{0_mm, 0_mm, hlZ});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  // Test with Gap for negative z and Expand for positive z
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisZ, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Gap, VolumeResizeStrategy::Expand}, *logger);

  // Update bounds to test only positive z expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin, rMax, 3 * hlZ);
  Transform3 newTransform =
      Transform3::Identity() * Translation3{0_mm, 0_mm, hlZ};
  cylStack.update(newBounds, newTransform, *logger);
  // Check that first volume maintains its size and position
  auto* firstVol = volumes.front();
  BOOST_CHECK_EQUAL(firstVol, vol1.get());
  auto firstBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&firstVol->volumeBounds());
  BOOST_REQUIRE(firstBounds != nullptr);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(firstVol->center()[eZ], -hlZ, 1e-10);

  // Check that second volume was expanded in positive z
  auto* lastVol = volumes.back();
  BOOST_CHECK_EQUAL(lastVol, vol2.get());
  auto lastBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&lastVol->volumeBounds());
  BOOST_REQUIRE(lastBounds != nullptr);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    2 * hlZ);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(lastVol->center()[eZ], 2 * hlZ, 1e-10);

  // No gap volumes should be created since only positive z changed
  BOOST_CHECK_EQUAL(volumes.size(), 2);
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeZFlipped) {
  double hlZ = 400_mm;
  double rMin = 100_mm;
  double rMax = 200_mm;

  // Create two cylinder volumes stacked in z
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);

  Transform3 transform1 = Transform3::Identity();
  transform1.translate(Vector3{0_mm, 0_mm, -hlZ});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = Transform3::Identity();
  transform2.translate(Vector3{0_mm, 0_mm, hlZ});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};
  // Test with Expand for negative z and Gap for positive z
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisZ, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Expand, VolumeResizeStrategy::Gap}, *logger);

  // Update bounds to test only positive z expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin, rMax, 3 * hlZ);
  Transform3 newTransform =
      Transform3::Identity() * Translation3{0_mm, 0_mm, hlZ};
  cylStack.update(newBounds, newTransform, *logger);
  // Check that first volume maintains its size and position
  auto* firstVol = volumes.front();
  BOOST_CHECK_EQUAL(firstVol, vol1.get());
  auto firstBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&firstVol->volumeBounds());
  BOOST_REQUIRE(firstBounds != nullptr);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(firstVol->center()[eZ], -hlZ, 1e-10);

  // Check that second volume stays the same
  auto* midVol = volumes[1];
  BOOST_CHECK_EQUAL(midVol, vol2.get());
  auto midBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&midVol->volumeBounds());
  BOOST_REQUIRE(midBounds != nullptr);
  BOOST_CHECK_EQUAL(midBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(midBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(midBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(midVol->center()[eZ], hlZ, 1e-10);

  // A gap volume should be created at positive z
  BOOST_CHECK_EQUAL(volumes.size(), 3);  // 2 volumes + 1 gap volume

  // Check gap volume at positive z
  auto* gapVol = volumes.back();
  auto gapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&gapVol->volumeBounds());
  BOOST_REQUIRE(gapBounds != nullptr);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(gapVol->center()[eZ], 3 * hlZ, 1e-10);
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeR) {
  double hlZ = 400_mm;
  double rMin1 = 100_mm;
  double rMax1 = 200_mm;
  double rMin2 = 200_mm;
  double rMax2 = 300_mm;

  // Create two cylinder volumes stacked in r
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin1, rMax1, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin2, rMax2, hlZ);

  Transform3 transform = Transform3::Identity();
  auto vol1 = std::make_shared<Volume>(transform, bounds1);
  auto vol2 = std::make_shared<Volume>(transform, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};

  // Test with Gap for inner radius and Expand for outer radius
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisR, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Gap, VolumeResizeStrategy::Expand}, *logger);

  // Update bounds to test only outer radius expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin1, 500_mm, hlZ);
  cylStack.update(newBounds, std::nullopt, *logger);

  // Check that inner volume maintains its size
  auto* innerVol = volumes.front();
  BOOST_CHECK_EQUAL(innerVol, vol1.get());
  auto innerBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&innerVol->volumeBounds());
  BOOST_REQUIRE(innerBounds != nullptr);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eMinR), rMin1);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eMaxR), rMax1);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

  // Check that outer volume was expanded only in outer radius
  auto* outerVol = volumes.back();
  BOOST_CHECK_EQUAL(outerVol, vol2.get());
  auto outerBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&outerVol->volumeBounds());
  BOOST_REQUIRE(outerBounds != nullptr);
  BOOST_CHECK_EQUAL(outerBounds->get(CylinderVolumeBounds::eMinR), rMin2);
  BOOST_CHECK_EQUAL(outerBounds->get(CylinderVolumeBounds::eMaxR), 500_mm);
  BOOST_CHECK_EQUAL(outerBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);

  // No gap volumes should be created since only outer radius changed
  BOOST_CHECK_EQUAL(volumes.size(), 2);
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeRFlipped) {
  double hlZ = 400_mm;
  double rMin1 = 100_mm;
  double rMax1 = 200_mm;
  double rMin2 = 200_mm;
  double rMax2 = 300_mm;

  // Create two cylinder volumes stacked in r
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin1, rMax1, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin2, rMax2, hlZ);

  Transform3 transform = Transform3::Identity();
  auto vol1 = std::make_shared<Volume>(transform, bounds1);
  auto vol2 = std::make_shared<Volume>(transform, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};
  // Test with Expand for inner radius and Gap for outer radius
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisR, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Expand, VolumeResizeStrategy::Gap}, *logger);

  // Update bounds to test only outer radius expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin1, 500_mm, hlZ);
  cylStack.update(newBounds, std::nullopt, *logger);
  // Check that inner volume maintains its size
  auto* innerVol = volumes.front();
  BOOST_CHECK_EQUAL(innerVol, vol1.get());
  auto innerBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&innerVol->volumeBounds());
  BOOST_REQUIRE(innerBounds != nullptr);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eMinR), rMin1);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eMaxR), rMax1);
  BOOST_CHECK_EQUAL(innerBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_CLOSE(innerVol->center()[eZ], 0, 1e-10);

  // Check that second volume maintains its size
  auto* midVol = volumes[1];
  BOOST_CHECK_EQUAL(midVol, vol2.get());
  auto midBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&midVol->volumeBounds());
  BOOST_REQUIRE(midBounds != nullptr);
  BOOST_CHECK_EQUAL(midBounds->get(CylinderVolumeBounds::eMinR), rMin2);
  BOOST_CHECK_EQUAL(midBounds->get(CylinderVolumeBounds::eMaxR), rMax2);
  BOOST_CHECK_EQUAL(midBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_CLOSE(midVol->center()[eZ], 0, 1e-10);

  // A gap volume should be created at outer radius
  BOOST_CHECK_EQUAL(volumes.size(), 3);  // 2 volumes + 1 gap volume

  // Check gap volume at positive z
  auto* gapVol = volumes.back();
  auto gapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&gapVol->volumeBounds());
  BOOST_REQUIRE(gapBounds != nullptr);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), rMax2);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), 500_mm);
  BOOST_CHECK_CLOSE(gapVol->center()[eZ], 0, 1e-10);
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeZNegative) {
  double hlZ = 400_mm;
  double rMin = 100_mm;
  double rMax = 200_mm;

  // Create two cylinder volumes stacked in z
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);

  Transform3 transform1 = Transform3::Identity();
  transform1.translate(Vector3{0_mm, 0_mm, -hlZ});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = Transform3::Identity();
  transform2.translate(Vector3{0_mm, 0_mm, hlZ});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};
  // Test with Gap for positive z and Expand for negative z
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisZ, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Expand, VolumeResizeStrategy::Gap}, *logger);

  // Update bounds to test only negative z expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin, rMax, 3 * hlZ);
  Transform3 newTransform =
      Transform3::Identity() * Translation3{0_mm, 0_mm, -hlZ};
  cylStack.update(newBounds, newTransform, *logger);
  // Check that first volume was expanded in negative z
  auto* firstVol = volumes.front();
  BOOST_CHECK_EQUAL(firstVol, vol1.get());
  auto firstBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&firstVol->volumeBounds());
  BOOST_REQUIRE(firstBounds != nullptr);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eHalfLengthZ),
                    2 * hlZ);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(firstVol->center()[eZ], -2 * hlZ, 1e-10);

  // Check that second volume maintains its size and position
  auto* lastVol = volumes.back();
  BOOST_CHECK_EQUAL(lastVol, vol2.get());
  auto lastBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&lastVol->volumeBounds());
  BOOST_REQUIRE(lastBounds != nullptr);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(lastVol->center()[eZ], hlZ, 1e-10);

  // No gap volumes should be created since only negative z changed
  BOOST_CHECK_EQUAL(volumes.size(), 2);
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeZNegativeFlipped) {
  double hlZ = 400_mm;
  double rMin = 100_mm;
  double rMax = 200_mm;

  // Create two cylinder volumes stacked in z
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin, rMax, hlZ);

  Transform3 transform1 = Transform3::Identity();
  transform1.translate(Vector3{0_mm, 0_mm, -hlZ});
  auto vol1 = std::make_shared<Volume>(transform1, bounds1);

  Transform3 transform2 = Transform3::Identity();
  transform2.translate(Vector3{0_mm, 0_mm, hlZ});
  auto vol2 = std::make_shared<Volume>(transform2, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};
  // Test with Gap for negative z and Expand for positive z
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisZ, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Gap, VolumeResizeStrategy::Expand}, *logger);

  // Update bounds to test only negative z expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(rMin, rMax, 3 * hlZ);
  Transform3 newTransform =
      Transform3::Identity() * Translation3{0_mm, 0_mm, -hlZ};
  cylStack.update(newBounds, newTransform, *logger);

  // A gap volume should be created at negative z
  BOOST_CHECK_EQUAL(volumes.size(), 3);  // 2 volumes + 1 gap volume

  // Check gap volume at negative z
  auto* gapVol = volumes[0];
  auto gapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&gapVol->volumeBounds());
  BOOST_REQUIRE(gapBounds != nullptr);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), rMin);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), rMax);
  BOOST_CHECK_CLOSE(gapVol->center()[eZ], -3 * hlZ, 1e-10);

  // Check that first original volume maintains its size and position
  auto* originalFirstVol = volumes[1];
  BOOST_CHECK_EQUAL(originalFirstVol, vol1.get());
  auto originalFirstBounds = dynamic_cast<const CylinderVolumeBounds*>(
      &originalFirstVol->volumeBounds());
  BOOST_REQUIRE(originalFirstBounds != nullptr);
  BOOST_CHECK_EQUAL(
      originalFirstBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(originalFirstBounds->get(CylinderVolumeBounds::eMinR),
                    rMin);
  BOOST_CHECK_EQUAL(originalFirstBounds->get(CylinderVolumeBounds::eMaxR),
                    rMax);
  BOOST_CHECK_CLOSE(originalFirstVol->center()[eZ], -hlZ, 1e-10);
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeRNegative) {
  double hlZ = 400_mm;
  double rMin1 = 100_mm;
  double rMax1 = 200_mm;
  double rMin2 = 200_mm;
  double rMax2 = 300_mm;

  // Create two cylinder volumes stacked in r
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin1, rMax1, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin2, rMax2, hlZ);

  Transform3 transform = Transform3::Identity();
  auto vol1 = std::make_shared<Volume>(transform, bounds1);
  auto vol2 = std::make_shared<Volume>(transform, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};
  // Test with Gap for outer radius and Expand for inner radius
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisR, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Expand, VolumeResizeStrategy::Gap}, *logger);

  // Update bounds to test only inner radius expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(50_mm, rMax2, hlZ);
  cylStack.update(newBounds, std::nullopt, *logger);
  // Check that first volume was expanded in inner radius
  auto* firstVol = volumes.front();
  BOOST_CHECK_EQUAL(firstVol, vol1.get());
  auto firstBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&firstVol->volumeBounds());
  BOOST_REQUIRE(firstBounds != nullptr);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eMaxR), rMax1);
  BOOST_CHECK_EQUAL(firstBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_CLOSE(firstVol->center()[eZ], 0, 1e-10);

  // Check that second volume maintains its size and position
  auto* lastVol = volumes.back();
  BOOST_CHECK_EQUAL(lastVol, vol2.get());
  auto lastBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&lastVol->volumeBounds());
  BOOST_REQUIRE(lastBounds != nullptr);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMinR), rMin2);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eMaxR), rMax2);
  BOOST_CHECK_EQUAL(lastBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_CLOSE(lastVol->center()[eZ], 0, 1e-10);

  // No gap volumes should be created since only inner radius changed
  BOOST_CHECK_EQUAL(volumes.size(), 2);
}

BOOST_AUTO_TEST_CASE(AsymmetricSingleSideResizeRNegativeFlipped) {
  double hlZ = 400_mm;
  double rMin1 = 100_mm;
  double rMax1 = 200_mm;
  double rMin2 = 200_mm;
  double rMax2 = 300_mm;

  // Create two cylinder volumes stacked in r
  auto bounds1 = std::make_shared<CylinderVolumeBounds>(rMin1, rMax1, hlZ);
  auto bounds2 = std::make_shared<CylinderVolumeBounds>(rMin2, rMax2, hlZ);

  Transform3 transform = Transform3::Identity();
  auto vol1 = std::make_shared<Volume>(transform, bounds1);
  auto vol2 = std::make_shared<Volume>(transform, bounds2);

  std::vector<Volume*> volumes = {vol1.get(), vol2.get()};
  // Test with Expand for outer radius and Gap for inner radius
  CylinderVolumeStack cylStack(
      volumes, AxisDirection::AxisR, VolumeAttachmentStrategy::Gap,
      {VolumeResizeStrategy::Gap, VolumeResizeStrategy::Expand}, *logger);

  // Update bounds to test only inner radius expansion
  auto newBounds = std::make_shared<CylinderVolumeBounds>(50_mm, rMax2, hlZ);
  cylStack.update(newBounds, std::nullopt, *logger);
  // A gap volume should be created at inner radius
  BOOST_CHECK_EQUAL(volumes.size(), 3);  // 2 volumes + 1 gap volume

  // Check gap volume at inner radius
  auto* gapVol = volumes[0];
  auto gapBounds =
      dynamic_cast<const CylinderVolumeBounds*>(&gapVol->volumeBounds());
  BOOST_REQUIRE(gapBounds != nullptr);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMinR), 50_mm);
  BOOST_CHECK_EQUAL(gapBounds->get(CylinderVolumeBounds::eMaxR), rMin1);
  BOOST_CHECK_CLOSE(gapVol->center()[eZ], 0, 1e-10);

  // Check that first original volume maintains its size and position
  auto* originalFirstVol = volumes[1];
  BOOST_CHECK_EQUAL(originalFirstVol, vol1.get());
  auto originalFirstBounds = dynamic_cast<const CylinderVolumeBounds*>(
      &originalFirstVol->volumeBounds());
  BOOST_REQUIRE(originalFirstBounds != nullptr);
  BOOST_CHECK_EQUAL(
      originalFirstBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_EQUAL(originalFirstBounds->get(CylinderVolumeBounds::eMinR),
                    rMin1);
  BOOST_CHECK_EQUAL(originalFirstBounds->get(CylinderVolumeBounds::eMaxR),
                    rMax1);
  BOOST_CHECK_CLOSE(originalFirstVol->center()[eZ], 0, 1e-10);

  // Check that second volume maintains its size and position
  auto* originalSecondVol = volumes[2];
  BOOST_CHECK_EQUAL(originalSecondVol, vol2.get());
  auto originalSecondBounds = dynamic_cast<const CylinderVolumeBounds*>(
      &originalSecondVol->volumeBounds());
  BOOST_REQUIRE(originalSecondBounds != nullptr);
  BOOST_CHECK_EQUAL(originalSecondBounds->get(CylinderVolumeBounds::eMinR),
                    rMin2);
  BOOST_CHECK_EQUAL(originalSecondBounds->get(CylinderVolumeBounds::eMaxR),
                    rMax2);
  BOOST_CHECK_EQUAL(
      originalSecondBounds->get(CylinderVolumeBounds::eHalfLengthZ), hlZ);
  BOOST_CHECK_CLOSE(originalSecondVol->center()[eZ], 0, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
