// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Detector/VolumeStructureBuilder.hpp"

#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/ConeVolumeBounds.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Utilities/Enumerate.hpp"

namespace {

/// This can be abandoned with C++20 to use the std::to_array metho
/// @param vecvals the vector of bound values to be converted
/// @return an array with the filled values
template <std::size_t kDIM>
std::array<Acts::ActsScalar, kDIM> to_array(
    const std::vector<Acts::ActsScalar>& vecvals) {
  std::array<Acts::ActsScalar, kDIM> rarray = {};
  for (const auto [iv, v] : Acts::enumerate(vecvals)) {
    if (iv < kDIM) {
      rarray[iv] = v;
    }
  }
  return rarray;
}
}  // namespace

Acts::Experimental::VolumeStructureBuilder::VolumeStructureBuilder(
    const Acts::Experimental::VolumeStructureBuilder::Config& cfg,
    std::unique_ptr<const Acts::Logger> logger)
    : IExternalStructureBuilder(), m_cfg(cfg), m_logger(std::move(logger)) {
  // Sanity cross-checks
  if (m_cfg.boundValues.empty() and not m_cfg.extent.has_value()) {
    throw std::invalid_argument(
        "VolumeStructureBuilder: no extent nor boundary values givgn");
  }
  // Check for the bounds type
  if (m_cfg.boundsType > VolumeBounds::BoundsType::eTrapezoid) {
    throw std::invalid_argument(
        "VolumeStructureBuilder: no bounds type provided.");
  }
}

Acts::Experimental::ExternalStructure
Acts::Experimental::VolumeStructureBuilder::construct(
    [[maybe_unused]] const Acts::GeometryContext& gctx) const {
  // The volume bounds to be constructed
  std::unique_ptr<VolumeBounds> volumeBounds = nullptr;

  // A transform
  auto transform = Transform3::Identity();
  std::vector<ActsScalar> boundValues = m_cfg.boundValues;

  // This code dispatches into the dedicated volume types
  switch (m_cfg.boundsType) {
    case VolumeBounds::BoundsType::eCone: {
      ACTS_VERBOSE("Building conical volume bounds.");
      // Cone translation - only pre-defined values
      if (boundValues.size() < 5u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cone volume bounds need to "
            "be fully provided, they can not be estimated from an Extent "
            "object.");
      }
      auto bArray = to_array<ConeVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<ConeVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eCuboid: {
      ACTS_VERBOSE("Building cuboid volume bounds.");
      // Cuboid translation - either parameters / or extent
      if (boundValues.empty() and m_cfg.extent.has_value()) {
        ACTS_VERBOSE("Cuboid: estimate parameters from Extent.");
        const auto& vExtent = m_cfg.extent.value();
        if (vExtent.constrains(binX) and vExtent.constrains(binY) and
            vExtent.constrains(binZ)) {
          transform.pretranslate(Vector3(vExtent.medium(binX),
                                         vExtent.medium(binY),
                                         vExtent.medium(binZ)));
          boundValues = {0.5 * vExtent.interval(binX),
                         0.5 * vExtent.interval(binY),
                         0.5 * vExtent.interval(binZ)};

        } else {
          throw std::runtime_error(
              "VolumeStructureBuilder: translation to cuboid does not work as "
              "the extent does not constrain all necessary value.");
        }
      } else if (boundValues.size() < 3u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cone volume bounds need to "
            "be fully provided, they can not be estimated from an Extent "
            "object.");
      }
      auto bArray =
          to_array<CuboidVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<CuboidVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eCutoutCylinder: {
      ACTS_VERBOSE("Building cutout cylindrical volume bounds.");
      // Cutout cylinder translation - only parameters
      if (boundValues.size() < 5u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cutout cylinder volume "
            "bounds need to be fully provided, they can not be estimated from "
            "an Extent object.");
      }
      auto bArray =
          to_array<CutoutCylinderVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<CutoutCylinderVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eCylinder: {
      ACTS_VERBOSE("Building cylindrical volume bounds.");
      // Cylinder translation - either parameters / or extent
      if (boundValues.empty() and m_cfg.extent.has_value()) {
        ACTS_VERBOSE("Cylinder: estimate parameters from Extent.");
        const auto& vExtent = m_cfg.extent.value();
        if (vExtent.constrains(binR) and vExtent.constrains(binZ)) {
          transform.pretranslate(Vector3(0., 0., vExtent.medium(binZ)));
          boundValues = {vExtent.min(binR), vExtent.max(binR),
                         0.5 * vExtent.interval(binZ)};
          if (vExtent.constrains(binPhi)) {
            boundValues.push_back(0.5 * vExtent.interval(binPhi));
            boundValues.push_back(vExtent.medium(binPhi));
          } else {
            boundValues.push_back(M_PI);
            boundValues.push_back(0.);
          }
        } else {
          throw std::runtime_error(
              "VolumeStructureBuilder: translation to cuboid does not work as "
              "the extent does not constrain all necessary value.");
        }
      } else if (boundValues.size() < 3u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cylinder volume "
            "bounds need to be fully provided.");
      }
      auto bArray =
          to_array<CylinderVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<CylinderVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eGenericCuboid: {
      ACTS_VERBOSE("Building generic cuboid volume bounds.");
      // Generic cuboid translation - parameters only
      if (boundValues.size() < GenericCuboidVolumeBounds::BoundValues::eSize) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for generic cuboid volume "
            "bounds need to be provided, they can not be estimated from an "
            "Extent object.");
      }
      auto bArray =
          to_array<GenericCuboidVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<GenericCuboidVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eTrapezoid: {
      ACTS_VERBOSE("Building trapezoid volume bounds.");
      // Trapezoid translation - parameters only
      if (boundValues.size() < 4u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for trapezoid volume bounds "
            "need to be provided, they can not be estimated from an Extent "
            "object.");
      }
      auto bArray =
          to_array<TrapezoidVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<TrapezoidVolumeBounds>(bArray);
    } break;
    default:
      break;
  }
  // Return the transform, the volume bounds, and some default portal
  // generators
  return {transform, std::move(volumeBounds), defaultPortalGenerator()};
}
