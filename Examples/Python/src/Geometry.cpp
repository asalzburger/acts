// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/CuboidalContainerBuilder.hpp"
#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/GapVolumeFiller.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/IndexedRootVolumeFinderBuilder.hpp"
#include "Acts/Detector/KdtSurfacesProvider.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Detector/interface/IDetectorBuilder.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Detector/interface/IDetectorManipulator.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IRootVolumeFinderBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/RangeXD.hpp"
#include "ActsExamples/Geometry/VolumeAssociationTest.hpp"

#include <array>
#include <memory>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace {
struct GeometryIdentifierHookBinding : public Acts::GeometryIdentifierHook {
  py::object callable;

  Acts::GeometryIdentifier decorateIdentifier(
      Acts::GeometryIdentifier identifier,
      const Acts::Surface& surface) const override {
    return callable(identifier, surface.getSharedPtr())
        .cast<Acts::GeometryIdentifier>();
  }
};

struct MaterialSurfaceSelector {
  std::vector<const Acts::Surface*> surfaces = {};

  /// @param surface is the test surface
  void operator()(const Acts::Surface* surface) {
    if (surface->surfaceMaterial() != nullptr) {
      if (std::find(surfaces.begin(), surfaces.end(), surface) ==
          surfaces.end()) {
        surfaces.push_back(surface);
      }
    }
  }
};

}  // namespace

namespace Acts::Python {
void addGeometry(Context& ctx) {
  auto m = ctx.get("main");

  {
    py::class_<Acts::GeometryIdentifier>(m, "GeometryIdentifier")
        .def(py::init<>())
        .def(py::init<Acts::GeometryIdentifier::Value>())
        .def("setVolume", &Acts::GeometryIdentifier::setVolume)
        .def("setLayer", &Acts::GeometryIdentifier::setLayer)
        .def("setBoundary", &Acts::GeometryIdentifier::setBoundary)
        .def("setApproach", &Acts::GeometryIdentifier::setApproach)
        .def("setSensitive", &Acts::GeometryIdentifier::setSensitive)
        .def("setExtra", &Acts::GeometryIdentifier::setExtra)
        .def("volume", &Acts::GeometryIdentifier::volume)
        .def("layer", &Acts::GeometryIdentifier::layer)
        .def("boundary", &Acts::GeometryIdentifier::boundary)
        .def("approach", &Acts::GeometryIdentifier::approach)
        .def("sensitive", &Acts::GeometryIdentifier::sensitive)
        .def("extra", &Acts::GeometryIdentifier::extra)
        .def("value", &Acts::GeometryIdentifier::value);
  }

  {
    py::class_<Acts::Surface, std::shared_ptr<Acts::Surface>>(m, "Surface")
        .def("geometryId",
             [](Acts::Surface& self) { return self.geometryId(); })
        .def("center",
             [](Acts::Surface& self) {
               return self.center(Acts::GeometryContext{});
             })
        .def("type", [](Acts::Surface& self) { return self.type(); });
  }

  {
    py::enum_<Acts::Surface::SurfaceType>(m, "SurfaceType")
        .value("Cone", Acts::Surface::SurfaceType::Cone)
        .value("Cylinder", Acts::Surface::SurfaceType::Cylinder)
        .value("Disc", Acts::Surface::SurfaceType::Disc)
        .value("Perigee", Acts::Surface::SurfaceType::Perigee)
        .value("Plane", Acts::Surface::SurfaceType::Plane)
        .value("Straw", Acts::Surface::SurfaceType::Straw)
        .value("Curvilinear", Acts::Surface::SurfaceType::Curvilinear)
        .value("Other", Acts::Surface::SurfaceType::Other);
  }

  {
    py::enum_<Acts::VolumeBounds::BoundsType>(m, "VolumeBoundsType")
        .value("Cone", Acts::VolumeBounds::BoundsType::eCone)
        .value("Cuboid", Acts::VolumeBounds::BoundsType::eCuboid)
        .value("CutoutCylinder",
               Acts::VolumeBounds::BoundsType::eCutoutCylinder)
        .value("Cylinder", Acts::VolumeBounds::BoundsType::eCylinder)
        .value("GenericCuboid", Acts::VolumeBounds::BoundsType::eGenericCuboid)
        .value("Trapezoid", Acts::VolumeBounds::BoundsType::eTrapezoid)
        .value("Other", Acts::VolumeBounds::BoundsType::eOther);
  }

  {
    py::class_<Acts::TrackingGeometry, std::shared_ptr<Acts::TrackingGeometry>>(
        m, "TrackingGeometry")
        .def("visitSurfaces",
             [](Acts::TrackingGeometry& self, py::function& func) {
               self.visitSurfaces(func);
             })
        .def("extractMaterialSurfaces",
             [](Acts::TrackingGeometry& self) {
               MaterialSurfaceSelector selector;
               self.visitSurfaces(selector, false);
               return selector.surfaces;
             })
        .def_property_readonly(
            "worldVolume",
            &Acts::TrackingGeometry::highestTrackingVolumeShared);
  }

  {
    py::class_<Acts::Volume, std::shared_ptr<Acts::Volume>>(m, "Volume")
        .def_static(
            "makeCylinderVolume",
            [](double r, double halfZ) {
              auto bounds =
                  std::make_shared<Acts::CylinderVolumeBounds>(0, r, halfZ);
              return std::make_shared<Acts::Volume>(Transform3::Identity(),
                                                    bounds);
            },
            "r"_a, "halfZ"_a);
  }

  {
    py::class_<Acts::TrackingVolume, Acts::Volume,
               std::shared_ptr<Acts::TrackingVolume>>(m, "TrackingVolume");
  }

  {
    py::class_<Acts::GeometryIdentifierHook,
               std::shared_ptr<Acts::GeometryIdentifierHook>>(
        m, "GeometryIdentifierHook")
        .def(py::init([](py::object callable) {
          auto hook = std::make_shared<GeometryIdentifierHookBinding>();
          hook->callable = callable;
          return hook;
        }));
  }

  {
    py::class_<Acts::Extent>(m, "Extent")
        .def(py::init(
            [](const std::vector<std::tuple<Acts::BinningValue,
                                            std::array<Acts::ActsScalar, 2u>>>&
                   franges) {
              Acts::Extent extent;
              for (const auto& [bval, frange] : franges) {
                extent.set(bval, frange[0], frange[1]);
              }
              return extent;
            }))
        .def("range", [](const Acts::Extent& self, Acts::BinningValue bval) {
          return std::array<Acts::ActsScalar, 2u>{self.min(bval),
                                                  self.max(bval)};
        });
  }
}

void addExperimentalGeometry(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  using namespace Acts::Experimental;

  // Detector volume definition
  py::class_<DetectorVolume, std::shared_ptr<DetectorVolume>>(m,
                                                              "DetectorVolume");

  // Detector definition
  py::class_<Detector, std::shared_ptr<Detector>>(m, "Detector")
      .def("numberVolumes",
           [](Detector& self) { return self.volumes().size(); })
      .def("extractMaterialSurfaces", [](Detector& self) {
        MaterialSurfaceSelector selector;
        self.visitSurfaces(selector);
        return selector.surfaces;
      });

  // Portal definition
  py::class_<Portal, std::shared_ptr<Portal>>(m, "Portal");

  {
    // The surface hierarchy map
    using SurfaceHierarchyMap =
        Acts::GeometryHierarchyMap<std::shared_ptr<Surface>>;

    py::class_<SurfaceHierarchyMap, std::shared_ptr<SurfaceHierarchyMap>>(
        m, "SurfaceHierarchyMap");

    // Extract volume / layer surfaces
    mex.def("extractVolumeLayerSurfaces", [](const SurfaceHierarchyMap& smap,
                                             bool sensitiveOnly) {
      std::map<unsigned int,
               std::map<unsigned int, std::vector<std::shared_ptr<Surface>>>>
          surfaceVolumeLayerMap;
      for (const auto& surface : smap) {
        auto gid = surface->geometryId();
        // Exclusion criteria
        if (sensitiveOnly and gid.sensitive() == 0) {
          continue;
        };
        surfaceVolumeLayerMap[gid.volume()][gid.layer()].push_back(surface);
      }
      // Return the surface volume map
      return surfaceVolumeLayerMap;
    });
  }

  {
    // Be able to construct a proto binning
    py::class_<ProtoBinning>(m, "ProtoBinning")
        .def(py::init<Acts::BinningValue, Acts::detail::AxisBoundaryType,
                      const std::vector<Acts::ActsScalar>&, std::size_t>())
        .def(py::init<Acts::BinningValue, Acts::detail::AxisBoundaryType,
                      Acts::ActsScalar, Acts::ActsScalar, std::size_t,
                      std::size_t>());
  }

  {
    // The internal layer structure builder
    py::class_<Acts::Experimental::IInternalStructureBuilder,
               std::shared_ptr<Acts::Experimental::IInternalStructureBuilder>>(
        m, "IInternalStructureBuilder");

    auto lsBuilder =
        py::class_<LayerStructureBuilder,
                   Acts::Experimental::IInternalStructureBuilder,
                   std::shared_ptr<LayerStructureBuilder>>(
            m, "LayerStructureBuilder")
            .def(py::init([](const LayerStructureBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<LayerStructureBuilder>(
                  config, getDefaultLogger(name, level));
            }));

    auto lsConfig =
        py::class_<LayerStructureBuilder::Config>(lsBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(lsConfig, LayerStructureBuilder::Config);
    ACTS_PYTHON_MEMBER(surfacesProvider);
    ACTS_PYTHON_MEMBER(supports);
    ACTS_PYTHON_MEMBER(binnings);
    ACTS_PYTHON_MEMBER(nSegments);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();

    // The internal layer structure builder
    py::class_<Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<Acts::Experimental::ISurfacesProvider>>(
        m, "ISurfacesProvider");

    py::class_<LayerStructureBuilder::SurfacesHolder,
               Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<LayerStructureBuilder::SurfacesHolder>>(
        lsBuilder, "SurfacesHolder")
        .def(py::init<std::vector<std::shared_ptr<Surface>>>());
  }

  {
    using RangeXDDim1 = Acts::RangeXD<1u, Acts::ActsScalar>;
    using KdtSurfacesDim1Bin100 = Acts::Experimental::KdtSurfaces<1u, 100u>;
    using KdtSurfacesProviderDim1Bin100 =
        Acts::Experimental::KdtSurfacesProvider<1u, 100u>;

    py::class_<RangeXDDim1>(m, "RangeXDDim1")
        .def(py::init([](const std::array<Acts::ActsScalar, 2u>& irange) {
          RangeXDDim1 range;
          range[0].shrink(irange[0], irange[1]);
          return range;
        }));

    py::class_<KdtSurfacesDim1Bin100, std::shared_ptr<KdtSurfacesDim1Bin100>>(
        m, "KdtSurfacesDim1Bin100")
        .def(py::init<const GeometryContext&,
                      const std::vector<std::shared_ptr<Acts::Surface>>&,
                      const std::array<Acts::BinningValue, 1u>&>())
        .def("surfaces", py::overload_cast<const RangeXDDim1&>(
                             &KdtSurfacesDim1Bin100::surfaces, py::const_));

    py::class_<KdtSurfacesProviderDim1Bin100,
               Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<KdtSurfacesProviderDim1Bin100>>(
        m, "KdtSurfacesProviderDim1Bin100")
        .def(py::init<std::shared_ptr<KdtSurfacesDim1Bin100>, const Extent&>());
  }

  {
    using RangeXDDim2 = Acts::RangeXD<2u, Acts::ActsScalar>;
    using KdtSurfacesDim2Bin100 = Acts::Experimental::KdtSurfaces<2u, 100u>;
    using KdtSurfacesProviderDim2Bin100 =
        Acts::Experimental::KdtSurfacesProvider<2u, 100u>;

    py::class_<RangeXDDim2>(m, "RangeXDDim2")
        .def(py::init([](const std::array<Acts::ActsScalar, 2u>& range0,
                         const std::array<Acts::ActsScalar, 2u>& range1) {
          RangeXDDim2 range;
          range[0].shrink(range0[0], range0[1]);
          range[1].shrink(range1[0], range1[1]);
          return range;
        }));

    py::class_<KdtSurfacesDim2Bin100, std::shared_ptr<KdtSurfacesDim2Bin100>>(
        m, "KdtSurfacesDim2Bin100")
        .def(py::init<const GeometryContext&,
                      const std::vector<std::shared_ptr<Acts::Surface>>&,
                      const std::array<Acts::BinningValue, 2u>&>())
        .def("surfaces", py::overload_cast<const RangeXDDim2&>(
                             &KdtSurfacesDim2Bin100::surfaces, py::const_));

    py::class_<KdtSurfacesProviderDim2Bin100,
               Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<KdtSurfacesProviderDim2Bin100>>(
        m, "KdtSurfacesProviderDim2Bin100")
        .def(py::init<std::shared_ptr<KdtSurfacesDim2Bin100>, const Extent&>());
  }

  {
    // The external volume structure builder
    py::class_<Acts::Experimental::IExternalStructureBuilder,
               std::shared_ptr<Acts::Experimental::IExternalStructureBuilder>>(
        m, "IExternalStructureBuilder");

    auto vsBuilder =
        py::class_<VolumeStructureBuilder,
                   Acts::Experimental::IExternalStructureBuilder,
                   std::shared_ptr<VolumeStructureBuilder>>(
            m, "VolumeStructureBuilder")
            .def(py::init([](const VolumeStructureBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<VolumeStructureBuilder>(
                  config, getDefaultLogger(name, level));
            }));

    auto vsConfig =
        py::class_<VolumeStructureBuilder::Config>(vsBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(vsConfig, VolumeStructureBuilder::Config);
    ACTS_PYTHON_MEMBER(boundsType);
    ACTS_PYTHON_MEMBER(boundValues);
    ACTS_PYTHON_MEMBER(transform);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::Experimental::IGeometryIdGenerator,
               std::shared_ptr<Acts::Experimental::IGeometryIdGenerator>>(
        m, "IGeometryIdGenerator");

    auto geoIdGen =
        py::class_<Acts::Experimental::GeometryIdGenerator,
                   Acts::Experimental::IGeometryIdGenerator,
                   std::shared_ptr<Acts::Experimental::GeometryIdGenerator>>(
            m, "GeometryIdGenerator")
            .def(py::init([](Acts::Experimental::GeometryIdGenerator::Config&
                                 config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<Acts::Experimental::GeometryIdGenerator>(
                  config, getDefaultLogger(name, level));
            }));

    auto geoIdGenConfig =
        py::class_<Acts::Experimental::GeometryIdGenerator::Config>(geoIdGen,
                                                                    "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(geoIdGenConfig,
                             Acts::Experimental::GeometryIdGenerator::Config);
    ACTS_PYTHON_MEMBER(containerMode);
    ACTS_PYTHON_MEMBER(containerId);
    ACTS_PYTHON_MEMBER(resetSubCounters);
    ACTS_PYTHON_MEMBER(overrideExistingIds);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // Put them together to a detector volume
    py::class_<Acts::Experimental::IDetectorComponentBuilder,
               std::shared_ptr<Acts::Experimental::IDetectorComponentBuilder>>(
        m, "IDetectorComponentBuilder");

    auto dvBuilder =
        py::class_<DetectorVolumeBuilder,
                   Acts::Experimental::IDetectorComponentBuilder,
                   std::shared_ptr<DetectorVolumeBuilder>>(
            m, "DetectorVolumeBuilder")
            .def(py::init([](const DetectorVolumeBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<DetectorVolumeBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &DetectorVolumeBuilder::construct);

    auto dvConfig =
        py::class_<DetectorVolumeBuilder::Config>(dvBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(dvConfig, DetectorVolumeBuilder::Config);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(internalsBuilder);
    ACTS_PYTHON_MEMBER(externalsBuilder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // The external volume structure builder
    py::class_<Acts::Experimental::IRootVolumeFinderBuilder,
               std::shared_ptr<Acts::Experimental::IRootVolumeFinderBuilder>>(
        m, "IRootVolumeFinderBuilder");

    auto irvBuilder =
        py::class_<Acts::Experimental::IndexedRootVolumeFinderBuilder,
                   Acts::Experimental::IRootVolumeFinderBuilder,
                   std::shared_ptr<
                       Acts::Experimental::IndexedRootVolumeFinderBuilder>>(
            m, "IndexedRootVolumeFinderBuilder")
            .def(py::init<std::vector<Acts::BinningValue>>());
  }

  {
    // Cylindrical container builder
    auto ccBuilder =
        py::class_<CylindricalContainerBuilder,
                   Acts::Experimental::IDetectorComponentBuilder,
                   std::shared_ptr<CylindricalContainerBuilder>>(
            m, "CylindricalContainerBuilder")
            .def(py::init([](const CylindricalContainerBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<CylindricalContainerBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &CylindricalContainerBuilder::construct);

    auto ccConfig =
        py::class_<CylindricalContainerBuilder::Config>(ccBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(ccConfig, CylindricalContainerBuilder::Config);
    ACTS_PYTHON_MEMBER(builders);
    ACTS_PYTHON_MEMBER(binning);
    ACTS_PYTHON_MEMBER(rootVolumeFinderBuilder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(geoIdReverseGen);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // Cuboidal container builder
    auto ccBuilder =
        py::class_<CuboidalContainerBuilder,
                   Acts::Experimental::IDetectorComponentBuilder,
                   std::shared_ptr<CuboidalContainerBuilder>>(
            m, "CuboidalContainerBuilder")
            .def(py::init([](const CuboidalContainerBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<CuboidalContainerBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &CuboidalContainerBuilder::construct);

    auto ccConfig =
        py::class_<CuboidalContainerBuilder::Config>(ccBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(ccConfig, CuboidalContainerBuilder::Config);
    ACTS_PYTHON_MEMBER(builders);
    ACTS_PYTHON_MEMBER(binning);
    ACTS_PYTHON_MEMBER(rootVolumeFinderBuilder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(geoIdReverseGen);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // Detector builder
    auto dBuilder =
        py::class_<DetectorBuilder, std::shared_ptr<DetectorBuilder>>(
            m, "DetectorBuilder")
            .def(py::init([](const DetectorBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<DetectorBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &DetectorBuilder::construct);

    auto dConfig = py::class_<DetectorBuilder::Config>(dBuilder, "Config")
                       .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(dConfig, DetectorBuilder::Config);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(builder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::Experimental::IDetectorManipulator,
               std::shared_ptr<Acts::Experimental::IDetectorManipulator>>(
        m, "IDetectorManipulator");

    auto gvFiller =
        py::class_<Acts::Experimental::GapVolumeFiller,
                   Acts::Experimental::IDetectorManipulator,
                   std::shared_ptr<Acts::Experimental::GapVolumeFiller>>(
            m, "GapVolumeFiller")
            .def(py::init(
                [](const Acts::Experimental::GapVolumeFiller::Config& config,
                   const std::string& name, Acts::Logging::Level level) {
                  return std::make_shared<Acts::Experimental::GapVolumeFiller>(
                      config, getDefaultLogger(name, level));
                }));

    auto gvConfig = py::class_<Acts::Experimental::GapVolumeFiller::Config>(
                        gvFiller, "Config")
                        .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(gvConfig,
                             Acts::Experimental::GapVolumeFiller::Config);
    ACTS_PYTHON_MEMBER(surfaces);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    mex.def("constructMaterialSurfacesODD", []() {
      std::vector<std::shared_ptr<Acts::Surface>> surfaces;

      /**
       * Cylinder format
       *
      ActsScalar r = 800;
      ActsScalar hz = 1150.;
      ActsScalar z = 0.;
      std::size_t nBinsZ = 100u;
      std::size_t nBinsPhi = 1u;
      **/
      using CylinderFormat = std::tuple<ActsScalar, ActsScalar, ActsScalar,
                                        std::size_t, std::size_t>;

      std::vector<CylinderFormat> cylinders = {

          // Pixels
          {42, 575, 0., 250u, 1u},
          {80, 575, 0., 250u, 1u},
          {129, 575, 0., 250u, 1u},
          {185, 575, 0., 250u, 1u},

          // Short Strips - barrels 
          {237, 1180, 0., 150u, 1u},
          {337, 1180, 0., 150u, 1u},
          {477, 1180, 0., 150u, 1u},
          {637, 1180, 0., 150u, 1u},

          // Inter short / long strip endcap
          {730, 90, -1460., 10u, 1u},
          {730, 90, 1460., 10u, 1u},
          {730, 100, -1760., 10u, 1u},
          {730, 100, 1760., 10u, 1u},
          {730, 110, -2080., 10u, 1u},
          {730, 110, 2080., 10u, 1u},
          {730, 120, -2430., 10u, 1u},
          {730, 120, 2430., 10u, 1u},
          {730, 130, -2790., 10u, 1u},
          {730, 130, 2790., 10u, 1u},

          // Long Strip section
          {800, 1180, 0., 100u, 1u},
          {1000, 1180, 0., 100u, 1u},

          // Solenoid
          { 1180, 3500., 0., 350u, 1u}

      };

      for (auto [r, hz, z, binsZ, binsPhi] : cylinders) {
        // Create the cylinder
        Transform3 transform = Transform3::Identity();
        transform.pretranslate(Vector3(0., 0., z));
        auto cylinder = Surface::makeShared<CylinderSurface>(transform, r, hz);
        // Add the material
        BinUtility binUtility(binsZ, -hz, hz, open, binZ, transform);
        if (binsPhi > 1) {
          binUtility += BinUtility(binsPhi, -M_PI, M_PI, closed, binPhi);
        }
        auto protoMaterial = std::make_shared<ProtoSurfaceMaterial>(binUtility);
        cylinder->assignSurfaceMaterial(protoMaterial);
        surfaces.push_back(cylinder);
      }

      // Endcap format
      /** ActsScalar ri ... inner radius
          ActsScalar ro ... outer radius
          array<ActsScalar, 2u> signs
          vector<ActsScalar> z positions
      std::size_t nBinsR= 100u;
      std::size_t nBinsPhi = 1u;
      */

      using DiscFormat =
          std::tuple<ActsScalar, ActsScalar, std::array<ActsScalar, 2u>, std::vector<ActsScalar>,
                     std::size_t, std::size_t>;

      std::vector<DiscFormat> discs = {

          // Pixels
          {33,
           193,
           {-1, 1},
           {590, 640, 740, 860, 1000, 1140, 1340, 1540, 2000},
           50u,
           1u},

          // Short Strips
          {210,
           710,
           {-1, 1},
           {1225, 1325, 1575, 1875, 2225, 2575, 2975},
           50u,
           1u},

          // Long Strips
          {740,
           1120,
           {-1, 1},
           {1225, 1350, 1650, 1950, 2300, 2650, 3050},
           50u,
           1u},          

      };

      // Add the discs
      for (auto [rI, rO, signs, zpositions, binsR, binsPhi] : discs) {
        // Bin utility to be done
        BinUtility binUtility(binsR, rI, rO, open, binR);
        if (binsPhi > 1) {
          binUtility += BinUtility(binsPhi, -M_PI, M_PI, closed, binPhi);
        }

        auto radialBounds = std::make_shared<RadialBounds>(rI, rO);

        for (auto s : signs) {
          for (auto z : zpositions) {
            // Create the disc
            Transform3 transform = Transform3::Identity();
            transform.pretranslate(Vector3(0., 0., s*z));
            auto disc =
                Surface::makeShared<DiscSurface>(transform, radialBounds);
            // Add the material
            auto protoMaterial =
                std::make_shared<ProtoSurfaceMaterial>(binUtility);
            disc->assignSurfaceMaterial(protoMaterial);
            surfaces.push_back(disc);
          }
        }
      }

      return surfaces;
    });

    /**
    std::vector<std::shared_ptr<Acts::Surface>> surfaces;
    auto lsBarrel1 = Surface::makeShared<CylinderSurface>(
        Transform3::Identity(), 800., 1150.);

    surfaces.push_back(lsBarrel1);
    return surfaces;
  });
  **/
  }

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::VolumeAssociationTest, mex,
                                "VolumeAssociationTest", name, ntests,
                                randomNumbers, randomRange, detector);
}

}  // namespace Acts::Python
