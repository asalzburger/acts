// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationStateUpdators.hpp"
#include "Acts/Navigation/SurfaceCandidatesUpdators.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <tuple>

namespace Acts {
namespace Experimental {

class Portal;

using DetectorVolumeExternals =
    std::tuple<Transform3, std::unique_ptr<CylinderVolumeBounds>>;

// Struct to convert ProtoVolumes into a cylincrical DetectorVolumes
struct ConcentricCylinderConverter {
  /// The proto volume that needs to be converted
  ProtoVolume protoVolume;

  ///  Create cylindrical volume bounds
  ///
  /// @return a tuple oa  tansform and cylinder volume bounds with it
  DetectorVolumeExternals create(const GeometryContext& /*ignored*/);
};

using DetectorVolumeInternals =
    std::tuple<std::vector<std::shared_ptr<Surface>>,
               std::vector<std::shared_ptr<DetectorVolume>>,
               SurfaceCandidatesUpdator>;

/// A struct that creates empty internals of a volume
struct EmptyInternals {
  /// The proto volume that needs to be converted, ignored here
  ProtoVolume protoVolume;

  /// @brief Create an internal structure for only portals
  ///
  /// @return a tuple of surfaces, volumes, updators
  DetectorVolumeInternals create(const GeometryContext& /* ingored */) {
    std::vector<std::shared_ptr<Surface>> noSurfaces = {};
    std::vector<std::shared_ptr<DetectorVolume>> noVolumes = {};
    auto portals = tryAllPortals();
    DetectorVolumeInternals rTuple = {std::move(noSurfaces),
                                      std::move(noVolumes), std::move(portals)};
    return rTuple;
  }
};

/// A struct that makes a default portal generator
struct DefaultPortalsConverter {
  /// The proto volume that needs to be converted, ignored here
  ProtoVolume protoVolume;

  /// This method creates a standard portal generator
  auto create(const GeometryContext& /* ingored */) {
    return defaultPortalGenerator();
  }
};

/// Definition of a shell builder, it builds a detector volume and
/// applies its shell to it
template <typename Volume = ConcentricCylinderConverter,
          typename Portals = DefaultPortalsConverter,
          typename Internals = EmptyInternals>
struct SingleBlockBuilder {
  /// The proto volume that needs to be converted
  ProtoVolume protoVolume;

  /// Convert a proto volume into a detector volume
  ///
  /// @tparam Volume how the volume is handled (bounds, position)
  /// @tparam Portals how the portals are handled
  /// @tparam InternalsHandling how the internals are handled
  ///
  /// @param dBlock The detector block to be built
  /// @param gctx The geometry context
  /// @param logLevel is a screen output log level
  ///
  /// @return a newly created DetectorVolume
  void operator()(DetectorBlock& dBlock, const GeometryContext& gctx,
                  Acts::Logging::Level logLevel = Acts::Logging::INFO) {
    // Screen output
    ACTS_LOCAL_LOGGER(getDefaultLogger(
        "SingleBlockBuilder   [ " + protoVolume.name + " ]", logLevel));

    ACTS_DEBUG("Building single volume '" << protoVolume.name << "'.");
    auto& dVolumes = std::get<DetectorVolumes>(dBlock);
    auto& dContainer = std::get<ProtoContainer>(dBlock);
    // Externals
    auto [transform, bounds] = Volume{protoVolume}.create(gctx);
    // Internals
    auto [surfaces, volumes, updator] = Internals{protoVolume}.create(gctx);
    // Portals
    auto portals = Portals{protoVolume}.create(gctx);
    // Construct the detector volume
    auto dVolume = DetectorVolumeFactory::construct(
        portals, gctx, protoVolume.name, transform, std::move(bounds), surfaces,
        volumes, std::move(updator));
    // Update the detector block
    dVolumes.push_back(dVolume);
    for (auto [ip, p] : enumerate(dVolume->portalPtrs())) {
      ACTS_VERBOSE(" - adding portal " << ip << " to the proto container.");
      dContainer[ip] = p;
    }
    ACTS_VERBOSE(" - total number of portals added: " << dContainer.size());
  }
};

struct ContainerBlockBuilder {
  /// The proto volume that needs to be converted
  ProtoVolume protoVolume;

  /// Convert a proto volume into a detector volume
  ///
  /// @tparam Volume how the volume is handled (bounds, position)
  /// @tparam Portals how the portals are handled
  /// @tparam InternalsHandling how the internals are handled
  ///
  /// @param dBlock [in,out] The detector block to be built
  /// @param gctx The geometry context
  /// @param logLevel is a sreen output log level
  ///
  /// @return a newly created DetectorVolume
  void operator()(DetectorBlock& dBlock, const GeometryContext& gctx,
                  Acts::Logging::Level logLevel = Acts::Logging::INFO);
};

}  // namespace Experimental
}  // namespace Acts
