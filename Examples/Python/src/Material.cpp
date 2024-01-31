// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Material/LegacySurfaceMaterialMapper.hpp"
#include "Acts/Material/LegacyVolumeMaterialMapper.hpp"
#include "Acts/Material/SurfaceMaterialMapper.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Io/Root/RootMaterialDecorator.hpp"
#include "ActsExamples/MaterialMapping/LegacyMaterialMapping.hpp"
#include "ActsExamples/MaterialMapping/MappingMaterialDecorator.hpp"
#include "ActsExamples/MaterialMapping/MaterialMapping.hpp"

#include <array>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Acts {
class TrackingGeometry;
}  // namespace Acts
namespace ActsExamples {
class IAlgorithm;
}  // namespace ActsExamples

namespace py = pybind11;
using namespace pybind11::literals;

using namespace ActsExamples;

namespace Acts::Python {
void addMaterial(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");
  {
    py::class_<Acts::IMaterialDecorator,
               std::shared_ptr<Acts::IMaterialDecorator>>(m,
                                                          "IMaterialDecorator");
  }

  {
    auto rmd =
        py::class_<RootMaterialDecorator, Acts::IMaterialDecorator,
                   std::shared_ptr<RootMaterialDecorator>>(
            mex, "RootMaterialDecorator")
            .def(
                py::init<RootMaterialDecorator::Config, Acts::Logging::Level>(),
                py::arg("config"), py::arg("level"));

    using Config = RootMaterialDecorator::Config;
    auto c = py::class_<Config>(rmd, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(voltag);
    ACTS_PYTHON_MEMBER(boutag);
    ACTS_PYTHON_MEMBER(laytag);
    ACTS_PYTHON_MEMBER(apptag);
    ACTS_PYTHON_MEMBER(sentag);
    ACTS_PYTHON_MEMBER(ntag);
    ACTS_PYTHON_MEMBER(vtag);
    ACTS_PYTHON_MEMBER(otag);
    ACTS_PYTHON_MEMBER(mintag);
    ACTS_PYTHON_MEMBER(maxtag);
    ACTS_PYTHON_MEMBER(ttag);
    ACTS_PYTHON_MEMBER(x0tag);
    ACTS_PYTHON_MEMBER(l0tag);
    ACTS_PYTHON_MEMBER(atag);
    ACTS_PYTHON_MEMBER(ztag);
    ACTS_PYTHON_MEMBER(rhotag);
    ACTS_PYTHON_MEMBER(fileName);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<MappingMaterialDecorator, Acts::IMaterialDecorator,
               std::shared_ptr<MappingMaterialDecorator>>(
        m, "MappingMaterialDecorator")
        .def(py::init<const Acts::TrackingGeometry&, Acts::Logging::Level, bool,
                      bool>(),
             py::arg("tGeometry"), py::arg("level"),
             py::arg("clearSurfaceMaterial") = true,
             py::arg("clearVolumeMaterial") = true)
        .def("binningMap", &MappingMaterialDecorator::binningMap)
        .def("setBinningMap", &MappingMaterialDecorator::setBinningMap);
  }

  {
    using Alg = ActsExamples::LegacyMaterialMapping;

    auto alg = py::class_<Alg, ActsExamples::IAlgorithm, std::shared_ptr<Alg>>(
                   mex, "LegacyMaterialMapping")
                   .def(py::init<const Alg::Config&, Acts::Logging::Level>(),
                        py::arg("config"), py::arg("level"))
                   .def("scoringParameters", &Alg::scoringParameters)
                   .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Alg::Config>(alg, "Config");

    ACTS_PYTHON_STRUCT_BEGIN(c, Alg::Config);
    ACTS_PYTHON_MEMBER(collection);
    ACTS_PYTHON_MEMBER(mappingMaterialCollection);
    ACTS_PYTHON_MEMBER(materialSurfaceMapper);
    ACTS_PYTHON_MEMBER(materialVolumeMapper);
    ACTS_PYTHON_MEMBER(materialWriters);
    ACTS_PYTHON_MEMBER(trackingGeometry);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using Alg = ActsExamples::MaterialMapping;
    auto alg = py::class_<Alg, ActsExamples::IAlgorithm, std::shared_ptr<Alg>>(
                   mex, "MaterialMapping")
                   .def(py::init<const Alg::Config&, Acts::Logging::Level>(),
                        py::arg("config"), py::arg("level"));

    auto c = py::class_<Alg::Config>(alg, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Alg::Config);
    ACTS_PYTHON_MEMBER(collection);
    ACTS_PYTHON_MEMBER(mappedMaterialCollection);
    ACTS_PYTHON_MEMBER(unmappedMaterialCollection);
    ACTS_PYTHON_MEMBER(materialMapper);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // The material mappers
    py::class_<Acts::IMaterialMapper, std::shared_ptr<Acts::IMaterialMapper>>(
        m, "IMaterialMapper");

    using SurfaceMapper = Acts::SurfaceMaterialMapper;
    auto sm =
        py::class_<SurfaceMapper, Acts::IMaterialMapper,
                   std::shared_ptr<SurfaceMapper>>(m, "SurfaceMaterialMapper")
            .def(py::init([](const SurfaceMapper::Config& config,
                             Acts::Logging::Level level) {
                   return std::make_shared<SurfaceMapper>(
                       config,
                       getDefaultLogger("SurfaceMaterialMapper", level));
                 }),
                 py::arg("config"), py::arg("level"));

    auto c = py::class_<SurfaceMapper::Config>(sm, "Config")
                 .def(py::init<>())
                 .def(py::init<const std::vector<const Acts::Surface*>&>(),
                      py::arg("surfaces"));

    ACTS_PYTHON_STRUCT_BEGIN(c, SurfaceMapper::Config);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto cls =
        py::class_<LegacySurfaceMaterialMapper,
                   std::shared_ptr<LegacySurfaceMaterialMapper>>(
            m, "LegacySurfaceMaterialMapper")
            .def(
                py::init(
                    [](const LegacySurfaceMaterialMapper::Config& config,
                       LegacySurfaceMaterialMapper::StraightLinePropagator prop,
                       Acts::Logging::Level level) {
                      return std::make_shared<LegacySurfaceMaterialMapper>(
                          config, std::move(prop),
                          getDefaultLogger("LegacySurfaceMaterialMapper",
                                           level));
                    }),
                py::arg("config"), py::arg("propagator"), py::arg("level"));

    auto c = py::class_<LegacySurfaceMaterialMapper::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, LegacySurfaceMaterialMapper::Config);
    ACTS_PYTHON_MEMBER(etaRange);
    ACTS_PYTHON_MEMBER(emptyBinCorrection);
    ACTS_PYTHON_MEMBER(mapperDebugOutput);
    ACTS_PYTHON_MEMBER(computeVariance);
    ACTS_PYTHON_MEMBER(trackingGeometry);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto cls =
        py::class_<LegacyVolumeMaterialMapper,
                   std::shared_ptr<LegacyVolumeMaterialMapper>>(
            m, "LegacyVolumeMaterialMapper")
            .def(py::init(
                     [](const LegacyVolumeMaterialMapper::Config& config,
                        LegacyVolumeMaterialMapper::StraightLinePropagator prop,
                        Acts::Logging::Level level) {
                       return std::make_shared<LegacyVolumeMaterialMapper>(
                           config, std::move(prop),
                           getDefaultLogger("LegacyVolumeMaterialMapper",
                                            level));
                     }),
                 py::arg("config"), py::arg("propagator"), py::arg("level"));

    auto c = py::class_<LegacyVolumeMaterialMapper::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, LegacyVolumeMaterialMapper::Config);
    ACTS_PYTHON_MEMBER(mappingStep);
    ACTS_PYTHON_MEMBER(trackingGeometry);
    ACTS_PYTHON_STRUCT_END();
  }
}
}  // namespace Acts::Python
