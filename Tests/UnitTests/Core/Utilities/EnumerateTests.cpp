// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/Enumerate.hpp"

BOOST_AUTO_TEST_CASE(EnumerateIndexing) {
  std::vector<std::string> values = {"one", "two", "three", "four"};
  std::vector<size_t> indices = {0, 1, 2, 3};

  for (auto [i, v] : Acts::enumerate(values)) {
    BOOST_CHECK(v == values[i]);
    BOOST_CHECK(i == indices[i]);
  }
}
