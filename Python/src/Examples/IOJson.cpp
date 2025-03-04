// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Json/JsonMaterialWriter.hpp"
#include "ActsExamples/Io/Json/JsonSurfacesWriter.hpp"
#include "ActsExamples/Io/Json/JsonTrackParamsLookupReader.hpp"
#include "ActsExamples/Io/Json/JsonTrackParamsLookupWriter.hpp"


{
    py::enum_<JsonFormat>(json, "Format")
        .value("NoOutput", JsonFormat::NoOutput)
        .value("Json", JsonFormat::Json)
        .value("Cbor", JsonFormat::Cbor)
        .value("All", JsonFormat::All);
  }


{
    auto cls =
        py::class_<JsonMaterialWriter, IMaterialWriter,
                   std::shared_ptr<JsonMaterialWriter>>(mex,
                                                        "JsonMaterialWriter")
            .def(py::init<const JsonMaterialWriter::Config&,
                          Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"))
            .def("writeMaterial", &JsonMaterialWriter::writeMaterial)
            .def("write", &JsonMaterialWriter::write)
            .def_property_readonly("config", &JsonMaterialWriter::config);

    auto c =
        py::class_<JsonMaterialWriter::Config>(cls, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, JsonMaterialWriter::Config);
    ACTS_PYTHON_MEMBER(converterCfg);
    ACTS_PYTHON_MEMBER(fileName);
    ACTS_PYTHON_MEMBER(writeFormat);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using IWriter = ActsExamples::ITrackParamsLookupWriter;
    using Writer = ActsExamples::JsonTrackParamsLookupWriter;
    using Config = Writer::Config;

    auto cls = py::class_<Writer, IWriter, std::shared_ptr<Writer>>(
                   mex, "JsonTrackParamsLookupWriter")
                   .def(py::init<const Config&>(), py::arg("config"))
                   .def("writeLookup", &Writer::writeLookup)
                   .def_property_readonly("config", &Writer::config);

    auto c = py::class_<Config>(cls, "Config")
                 .def(py::init<>())
                 .def(py::init<const std::string&>(), py::arg("path"));

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(path);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    using IReader = ActsExamples::ITrackParamsLookupReader;
    using Reader = ActsExamples::JsonTrackParamsLookupReader;
    using Config = Reader::Config;

    auto cls = py::class_<Reader, IReader, std::shared_ptr<Reader>>(
                   mex, "JsonTrackParamsLookupReader")
                   .def(py::init<const Config&>(), py::arg("config"))
                   .def("readLookup", &Reader::readLookup)
                   .def_property_readonly("config", &Reader::config);

    auto c = py::class_<Config>(cls, "Config")
                 .def(py::init<>())
                 .def(py::init<std::unordered_map<Acts::GeometryIdentifier,
                                                  const Acts::Surface*>,
                               std::pair<double, double>>(),
                      py::arg("refLayers"), py::arg("bins"));

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(refLayers);
    ACTS_PYTHON_MEMBER(bins);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto cls =
        py::class_<JsonSurfacesWriter, IWriter,
                   std::shared_ptr<JsonSurfacesWriter>>(mex,
                                                        "JsonSurfacesWriter")
            .def(py::init<const JsonSurfacesWriter::Config&,
                          Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"))
            .def("write", &JsonSurfacesWriter::write)
            .def_property_readonly("config", &JsonSurfacesWriter::config);

    auto c =
        py::class_<JsonSurfacesWriter::Config>(cls, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, JsonSurfacesWriter::Config);
    ACTS_PYTHON_MEMBER(trackingGeometry);
    ACTS_PYTHON_MEMBER(outputDir);
    ACTS_PYTHON_MEMBER(outputPrecision);
    ACTS_PYTHON_MEMBER(writeLayer);
    ACTS_PYTHON_MEMBER(writeApproach);
    ACTS_PYTHON_MEMBER(writeSensitive);
    ACTS_PYTHON_MEMBER(writeBoundary);
    ACTS_PYTHON_MEMBER(writePerEvent);
    ACTS_PYTHON_MEMBER(writeOnlyNames);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::ProtoDetector>(mex, "ProtoDetector")
        .def(py::init<>([](std::string pathName) {
          nlohmann::json jDetector;
          auto in = std::ifstream(pathName, std::ifstream::in);
          if (in.good()) {
            in >> jDetector;
            in.close();
          }
          Acts::ProtoDetector pDetector = jDetector["detector"];
          return pDetector;
        }));
  }
