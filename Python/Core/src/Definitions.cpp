// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/Definitions/Units.hpp"
#include "ActsPython/Utilities/Context.hpp"

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ActsPython {

/// This adds the Definitions python module to the context
void addDefinitions(Context& ctx) {
  auto& m = ctx.get("main");

  // Algebraic types
  py::class_<Acts::Vector2>(m, "Vector2")
      .def(py::init<double, double>())
      .def(py::init([](std::array<double, 2> a) {
        Acts::Vector2 v;
        v << a[0], a[1];
        return v;
      }))
      .def("__getitem__",
           [](const Acts::Vector2& self, Eigen::Index i) { return self[i]; })
      .def("__str__", [](const Acts::Vector3& self) {
        std::stringstream ss;
        ss << self.transpose();
        return ss.str();
      });

  py::class_<Acts::Vector3>(m, "Vector3")
      .def(py::init<double, double, double>())
      .def(py::init([](std::array<double, 3> a) {
        Acts::Vector3 v;
        v << a[0], a[1], a[2];
        return v;
      }))
      .def_static("UnitX",
                  []() -> Acts::Vector3 { return Acts::Vector3::UnitX(); })
      .def_static("UnitY",
                  []() -> Acts::Vector3 { return Acts::Vector3::UnitY(); })
      .def_static("UnitZ",
                  []() -> Acts::Vector3 { return Acts::Vector3::UnitZ(); })

      .def("__getitem__",
           [](const Acts::Vector3& self, Eigen::Index i) { return self[i]; })
      .def("__str__", [](const Acts::Vector3& self) {
        std::stringstream ss;
        ss << self.transpose();
        return ss.str();
      });

  py::class_<Acts::Vector4>(m, "Vector4")
      .def(py::init<double, double, double, double>())
      .def(py::init([](std::array<double, 4> a) {
        Acts::Vector4 v;
        v << a[0], a[1], a[2], a[3];
        return v;
      }))
      .def("__getitem__",
           [](const Acts::Vector4& self, Eigen::Index i) { return self[i]; });

  py::class_<Acts::Transform3>(m, "Transform3")
      .def(py::init<>())
      .def(py::init([](const Acts::Vector3& translation) -> Acts::Transform3 {
        return Acts::Transform3{Acts::Translation3{translation}};
      }))
      .def_property_readonly("translation",
                             [](const Acts::Transform3& self) -> Acts::Vector3 {
                               return self.translation();
                             })
      .def_static("Identity", &Acts::Transform3::Identity)
      .def("__mul__",
           [](const Acts::Transform3& self, const Acts::Transform3& other) {
             return self * other;
           })
      .def("__mul__",
           [](const Acts::Transform3& self, const Acts::Translation3& other) {
             return self * other;
           })
      .def("__mul__",
           [](const Acts::Transform3& self, const Acts::AngleAxis3& other) {
             return self * other;
           })
      .def("__str__", [](const Acts::Transform3& self) {
        std::stringstream ss;
        ss << self.matrix();
        return ss.str();
      });

  py::class_<Acts::Translation3>(m, "Translation3")
      .def(py::init(
          [](const Acts::Vector3& a) { return Acts::Translation3(a); }))
      .def(py::init([](std::array<double, 3> a) {
        return Acts::Translation3(Acts::Vector3(a[0], a[1], a[2]));
      }))
      .def("__str__", [](const Acts::Translation3& self) {
        std::stringstream ss;
        ss << self.translation().transpose();
        return ss.str();
      });

  py::class_<Acts::AngleAxis3>(m, "AngleAxis3")
      .def(py::init([](double angle, const Acts::Vector3& axis) {
        return Acts::AngleAxis3(angle, axis);
      }))
      .def("__str__", [](const Acts::Transform3& self) {
        std::stringstream ss;
        ss << self.matrix();
        return ss.str();
      });

  // Units
  auto u = m.def_submodule("UnitConstants");

#define UNIT(x) u.attr(#x) = Acts::UnitConstants::x;

  UNIT(fm)
  UNIT(pm)
  UNIT(um)
  UNIT(nm)
  UNIT(mm)
  UNIT(cm)
  UNIT(m)
  UNIT(km)
  UNIT(mm2)
  UNIT(cm2)
  UNIT(m2)
  UNIT(mm3)
  UNIT(cm3)
  UNIT(m3)
  UNIT(s)
  UNIT(fs)
  UNIT(ps)
  UNIT(ns)
  UNIT(us)
  UNIT(ms)
  UNIT(min)
  UNIT(h)
  UNIT(mrad)
  UNIT(rad)
  UNIT(degree)
  UNIT(eV)
  UNIT(keV)
  UNIT(MeV)
  UNIT(GeV)
  UNIT(TeV)
  UNIT(J)
  UNIT(u)
  UNIT(g)
  UNIT(kg)
  UNIT(e)
  UNIT(T)
  UNIT(Gauss)
  UNIT(kGauss)
  UNIT(mol)

#undef UNIT

  // Pdg Particle enums
  py::enum_<Acts::PdgParticle>(m, "PdgParticle")
      .value("eElectron", Acts::PdgParticle::eElectron)
      .value("ePositron", Acts::PdgParticle::ePositron)
      .value("eMuon", Acts::PdgParticle::eMuon)
      .value("eAntiMuon", Acts::PdgParticle::eAntiMuon)
      .value("ePionPlus", Acts::PdgParticle::ePionPlus)
      .value("ePionMinus", Acts::PdgParticle::ePionMinus)
      .value("eKaonPlus", Acts::PdgParticle::eKaonPlus)
      .value("eKaonMinus", Acts::PdgParticle::eKaonMinus)
      .value("eProton", Acts::PdgParticle::eProton)
      .value("eAntiProton", Acts::PdgParticle::eAntiProton)
      .value("eNeutron", Acts::PdgParticle::eNeutron)
      .value("eAntiNeutron", Acts::PdgParticle::eAntiNeutron);
}
}  // namespace ActsPython
