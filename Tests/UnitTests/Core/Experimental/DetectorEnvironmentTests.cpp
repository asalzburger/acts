// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Experimental/DetectorEnvironment.hpp"

#include <iostream>

namespace Acts {

namespace Test {

BOOST_AUTO_TEST_SUITE(Experimental)

/// Unit tests for Polyderon construction & operator +=
BOOST_AUTO_TEST_CASE(DetectorEnvironment_) {
  // Cosntruct an empty environment
  DetectorEnvironment dEnvironment;

  BOOST_CHECK(dEnvironment.currentVolume == nullptr);
  BOOST_CHECK(dEnvironment.currentSurface == nullptr);
  BOOST_CHECK(dEnvironment.surfaces.size() == 0u);
  BOOST_CHECK(dEnvironment.portals.size() == 0u);

  BOOST_CHECK(dEnvironment.status == DetectorEnvironment::eUninitialized);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Test
}  // namespace Acts