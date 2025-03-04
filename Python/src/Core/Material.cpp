// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/BinnedSurfaceMaterialAccumulater.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/IntersectionMaterialAssigner.hpp"
#include "Acts/Material/MaterialMapper.hpp"
#include "Acts/Material/MaterialValidater.hpp"
#include "Acts/Material/PropagatorMaterialAssigner.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Material/SurfaceMaterialMapper.hpp"
#include "Acts/Material/VolumeMaterialMapper.hpp"
#include "Acts/Python/PyUtilities.hpp"
#include "Acts/Utilities/Logger.hpp"

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

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {
void addMaterial(Context& ctx) {
  auto& m = ctx.get("main");

  {
    py::class_<Acts::ISurfaceMaterial, std::shared_ptr<ISurfaceMaterial>>(
        m, "ISurfaceMaterial")
        .def("toString", &Acts::ISurfaceMaterial::toString);

    py::class_<Acts::ProtoGridSurfaceMaterial, Acts::ISurfaceMaterial,
               std::shared_ptr<ProtoGridSurfaceMaterial>>(
        m, "ProtoGridSurfaceMaterial");

    py::class_<Acts::ProtoSurfaceMaterial, Acts::ISurfaceMaterial,
               std::shared_ptr<ProtoSurfaceMaterial>>(m,
                                                      "ProtoSurfaceMaterial");

    py::class_<Acts::HomogeneousSurfaceMaterial, Acts::ISurfaceMaterial,
               std::shared_ptr<HomogeneousSurfaceMaterial>>(
        m, "HomogeneousSurfaceMaterial");

    py::class_<Acts::IVolumeMaterial, std::shared_ptr<IVolumeMaterial>>(
        m, "IVolumeMaterial");
  }

  {
    py::class_<Acts::IMaterialDecorator,
               std::shared_ptr<Acts::IMaterialDecorator>>(m,
                                                          "IMaterialDecorator")
        .def("decorate", py::overload_cast<Surface&>(
                             &Acts::IMaterialDecorator::decorate, py::const_));
  }

  {
    auto cls =
        py::class_<SurfaceMaterialMapper,
                   std::shared_ptr<SurfaceMaterialMapper>>(
            m, "SurfaceMaterialMapper")
            .def(py::init([](const SurfaceMaterialMapper::Config& config,
                             SurfaceMaterialMapper::StraightLinePropagator prop,
                             Acts::Logging::Level level) {
                   return std::make_shared<SurfaceMaterialMapper>(
                       config, std::move(prop),
                       getDefaultLogger("SurfaceMaterialMapper", level));
                 }),
                 py::arg("config"), py::arg("propagator"), py::arg("level"));

    auto c = py::class_<SurfaceMaterialMapper::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, SurfaceMaterialMapper::Config);
    ACTS_PYTHON_MEMBER(etaRange);
    ACTS_PYTHON_MEMBER(emptyBinCorrection);
    ACTS_PYTHON_MEMBER(mapperDebugOutput);
    ACTS_PYTHON_MEMBER(computeVariance);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto cls =
        py::class_<VolumeMaterialMapper, std::shared_ptr<VolumeMaterialMapper>>(
            m, "VolumeMaterialMapper")
            .def(py::init([](const VolumeMaterialMapper::Config& config,
                             VolumeMaterialMapper::StraightLinePropagator prop,
                             Acts::Logging::Level level) {
                   return std::make_shared<VolumeMaterialMapper>(
                       config, std::move(prop),
                       getDefaultLogger("VolumeMaterialMapper", level));
                 }),
                 py::arg("config"), py::arg("propagator"), py::arg("level"));

    auto c = py::class_<VolumeMaterialMapper::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, VolumeMaterialMapper::Config);
    ACTS_PYTHON_MEMBER(mappingStep);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::IAssignmentFinder,
               std::shared_ptr<Acts::IAssignmentFinder>>(m,
                                                         "IAssignmentFinder");
  }

  {
    auto isma =
        py::class_<Acts::IntersectionMaterialAssigner, Acts::IAssignmentFinder,
                   std::shared_ptr<Acts::IntersectionMaterialAssigner>>(
            m, "IntersectionMaterialAssigner")
            .def(py::init([](const Acts::IntersectionMaterialAssigner::Config&
                                 config,
                             Acts::Logging::Level level) {
                   return std::make_shared<Acts::IntersectionMaterialAssigner>(
                       config,
                       getDefaultLogger("IntersectionMaterialAssigner", level));
                 }),
                 py::arg("config"), py::arg("level"))
            .def("assignmentCandidates",
                 &Acts::IntersectionMaterialAssigner::assignmentCandidates);

    auto c =
        py::class_<Acts::IntersectionMaterialAssigner::Config>(isma, "Config")
            .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Acts::IntersectionMaterialAssigner::Config);
    ACTS_PYTHON_MEMBER(surfaces);
    ACTS_PYTHON_MEMBER(trackingVolumes);
    ACTS_PYTHON_MEMBER(detectorVolumes);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::ISurfaceMaterialAccumulater,
               std::shared_ptr<Acts::ISurfaceMaterialAccumulater>>(
        m, "ISurfaceMaterialAccumulater");
  }

  {
    auto bsma =
        py::class_<BinnedSurfaceMaterialAccumulater,
                   ISurfaceMaterialAccumulater,
                   std::shared_ptr<BinnedSurfaceMaterialAccumulater>>(
            m, "BinnedSurfaceMaterialAccumulater")
            .def(
                py::init(
                    [](const BinnedSurfaceMaterialAccumulater::Config& config,
                       Acts::Logging::Level level) {
                      return std::make_shared<BinnedSurfaceMaterialAccumulater>(
                          config,
                          getDefaultLogger("BinnedSurfaceMaterialAccumulater",
                                           level));
                    }),
                py::arg("config"), py::arg("level"))
            .def("createState", &BinnedSurfaceMaterialAccumulater::createState)
            .def("accumulate", &BinnedSurfaceMaterialAccumulater::accumulate)
            .def("finalizeMaterial",
                 &BinnedSurfaceMaterialAccumulater::finalizeMaterial);

    auto c =
        py::class_<BinnedSurfaceMaterialAccumulater::Config>(bsma, "Config")
            .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, BinnedSurfaceMaterialAccumulater::Config);
    ACTS_PYTHON_MEMBER(emptyBinCorrection);
    ACTS_PYTHON_MEMBER(materialSurfaces);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto mm = py::class_<MaterialMapper, std::shared_ptr<MaterialMapper>>(
                  m, "MaterialMapper")
                  .def(py::init([](const MaterialMapper::Config& config,
                                   Acts::Logging::Level level) {
                         return std::make_shared<MaterialMapper>(
                             config, getDefaultLogger("MaterialMapper", level));
                       }),
                       py::arg("config"), py::arg("level"));

    auto c = py::class_<MaterialMapper::Config>(mm, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, MaterialMapper::Config);
    ACTS_PYTHON_MEMBER(assignmentFinder);
    ACTS_PYTHON_MEMBER(surfaceMaterialAccumulater);
    ACTS_PYTHON_STRUCT_END();
  }


}
}  // namespace Acts::Python
