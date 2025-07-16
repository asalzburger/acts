// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Utilities/Any.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsPython/Utilities/Context.hpp"

#include <array>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ActsPython {

void addContext(Context& ctx) {
  auto& m = ctx.get("main");

  py::class_<Acts::GeometryContext>(m, "GeometryContext").def(py::init<>());
  py::class_<Acts::MagneticFieldContext>(m, "MagneticFieldContext")
      .def(py::init<>());
  py::class_<Acts::CalibrationContext>(m, "CalibrationContext")
      .def(py::init<>());
}

void addAny(Context& ctx) {
  auto& m = ctx.get("main");

  py::class_<Acts::AnyBase<512>>(m, "AnyBase512").def(py::init<>());
}

void addUnits(Context& /*ctx*/) {}

class PythonLogger {
 public:
  PythonLogger(const std::string& name, Acts::Logging::Level level)
      : m_name{name}, m_logger{Acts::getDefaultLogger(m_name, level)} {}

  void log(Acts::Logging::Level level, const std::string& message) const {
    m_logger->log(level, message);
  }

  void setLevel(Acts::Logging::Level level) {
    m_logger = Acts::getDefaultLogger(m_name, level);
  }

 private:
  std::string m_name;
  std::unique_ptr<const Acts::Logger> m_logger;
};

void addLogging(ActsPython::Context& ctx) {
  auto& m = ctx.get("main");
  auto logging = m.def_submodule("logging", "");

  auto levelEnum = py::enum_<Acts::Logging::Level>(logging, "Level")
                       .value("VERBOSE", Acts::Logging::VERBOSE)
                       .value("DEBUG", Acts::Logging::DEBUG)
                       .value("INFO", Acts::Logging::INFO)
                       .value("WARNING", Acts::Logging::WARNING)
                       .value("ERROR", Acts::Logging::ERROR)
                       .value("FATAL", Acts::Logging::FATAL)
                       .value("MAX", Acts::Logging::MAX)
                       .export_values();

  levelEnum
      .def("__lt__", [](Acts::Logging::Level self,
                        Acts::Logging::Level other) { return self < other; })
      .def("__gt__", [](Acts::Logging::Level self,
                        Acts::Logging::Level other) { return self > other; })
      .def("__le__", [](Acts::Logging::Level self,
                        Acts::Logging::Level other) { return self <= other; })
      .def("__ge__", [](Acts::Logging::Level self, Acts::Logging::Level other) {
        return self >= other;
      });

  auto makeLogFunction = [](Acts::Logging::Level level) {
    return
        [level](PythonLogger& logger, const std::string& fmt, py::args args) {
          auto locals = py::dict();
          locals["args"] = args;
          locals["fmt"] = fmt;
          py::exec(R"(
        message = fmt % args
    )",
                   py::globals(), locals);

          auto message = locals["message"].cast<std::string>();

          logger.log(level, message);
        };
  };

  py::class_<Acts::Logger>(m, "Logger");

  auto logger =
      py::class_<PythonLogger, std::shared_ptr<PythonLogger>>(logging, "Logger")
          .def("log", &PythonLogger::log)
          .def("verbose", makeLogFunction(Acts::Logging::VERBOSE))
          .def("debug", makeLogFunction(Acts::Logging::DEBUG))
          .def("info", makeLogFunction(Acts::Logging::INFO))
          .def("warning", makeLogFunction(Acts::Logging::WARNING))
          .def("error", makeLogFunction(Acts::Logging::ERROR))
          .def("fatal", makeLogFunction(Acts::Logging::FATAL))
          .def("setLevel", &PythonLogger::setLevel);

  static std::unordered_map<std::string, std::shared_ptr<PythonLogger>>
      pythonLoggers = {{"root", std::make_shared<PythonLogger>(
                                    "Python", Acts::Logging::INFO)}};

  logging.def(
      "getLogger",
      [](const std::string& name) {
        if (!pythonLoggers.contains(name)) {
          pythonLoggers[name] =
              std::make_shared<PythonLogger>(name, Acts::Logging::INFO);
        }
        return pythonLoggers[name];
      },
      py::arg("name") = "root");

  logging.def("setLevel", [](Acts::Logging::Level level) {
    pythonLoggers.at("root")->setLevel(level);
  });

  auto makeModuleLogFunction = [](Acts::Logging::Level level) {
    return [level](const std::string& fmt, py::args args) {
      auto locals = py::dict();
      locals["args"] = args;
      locals["fmt"] = fmt;
      py::exec(R"(
        message = fmt % args
    )",
               py::globals(), locals);

      auto message = locals["message"].cast<std::string>();

      pythonLoggers.at("root")->log(level, message);
    };
  };

  logging.def("setFailureThreshold", &Acts::Logging::setFailureThreshold);
  logging.def("getFailureThreshold", &Acts::Logging::getFailureThreshold);

  struct ScopedFailureThresholdContextManager {
    std::optional<Acts::Logging::ScopedFailureThreshold>
        m_scopedFailureThreshold = std::nullopt;
    Acts::Logging::Level m_level;

    explicit ScopedFailureThresholdContextManager(Acts::Logging::Level level)
        : m_level(level) {}

    void enter() { m_scopedFailureThreshold.emplace(m_level); }

    void exit(const py::object& /*exc_type*/, const py::object& /*exc_value*/,
              const py::object& /*traceback*/) {
      m_scopedFailureThreshold.reset();
    }
  };

  py::class_<ScopedFailureThresholdContextManager>(logging,
                                                   "ScopedFailureThreshold")
      .def(py::init<Acts::Logging::Level>(), "level"_a)
      .def("__enter__", &ScopedFailureThresholdContextManager::enter)
      .def("__exit__", &ScopedFailureThresholdContextManager::exit);

  static py::exception<Acts::Logging::ThresholdFailure> exc(
      logging, "ThresholdFailure", PyExc_RuntimeError);
  // NOLINTNEXTLINE(performance-unnecessary-value-param)
  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if (p) {
        std::rethrow_exception(p);
      }
    } catch (const std::exception& e) {
      std::string what = e.what();
      if (what.find("ACTS_LOG_FAILURE_THRESHOLD") != std::string::npos) {
        py::set_error(exc, e.what());
      } else {
        std::rethrow_exception(p);
      }
    }
  });

  logging.def("verbose", makeModuleLogFunction(Acts::Logging::VERBOSE));
  logging.def("debug", makeModuleLogFunction(Acts::Logging::DEBUG));
  logging.def("info", makeModuleLogFunction(Acts::Logging::INFO));
  logging.def("warning", makeModuleLogFunction(Acts::Logging::WARNING));
  logging.def("error", makeModuleLogFunction(Acts::Logging::ERROR));
  logging.def("fatal", makeModuleLogFunction(Acts::Logging::FATAL));
}

void addPdgParticle(ActsPython::Context& /*ctx*/) {}

void addAlgebra(ActsPython::Context& /*ctx*/) {}
  

void addBinning(Context& ctx) {
  auto& m = ctx.get("main");

  auto binningValue = py::enum_<Acts::AxisDirection>(m, "AxisDirection")
                          .value("AxisX", Acts::AxisDirection::AxisX)
                          .value("AxisY", Acts::AxisDirection::AxisY)
                          .value("AxisZ", Acts::AxisDirection::AxisZ)
                          .value("AxisR", Acts::AxisDirection::AxisR)
                          .value("AxisPhi", Acts::AxisDirection::AxisPhi)
                          .value("AxisRPhi", Acts::AxisDirection::AxisRPhi)
                          .value("AxisTheta", Acts::AxisDirection::AxisTheta)
                          .value("AxisEta", Acts::AxisDirection::AxisEta)
                          .value("AxisMag", Acts::AxisDirection::AxisMag);

  auto boundaryType = py::enum_<Acts::AxisBoundaryType>(m, "AxisBoundaryType")
                          .value("Bound", Acts::AxisBoundaryType::Bound)
                          .value("Closed", Acts::AxisBoundaryType::Closed)
                          .value("Open", Acts::AxisBoundaryType::Open);

  auto axisType = py::enum_<Acts::AxisType>(m, "AxisType")
                      .value("equidistant", Acts::AxisType::Equidistant)
                      .value("variable", Acts::AxisType::Variable);
}

}  // namespace ActsPython
