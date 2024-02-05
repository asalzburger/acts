// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelReader.hpp"

#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelRead/ReadGeoModel.h>

Acts::GeoModelTree Acts::GeoModelReader::readFromDb(const std::string& dbPath) {
  // Data base manager
  GMDBManager* db = new GMDBManager(dbPath);
  if (!db->checkIsDBOpen()) {
    throw std::runtime_error("GeoModelReader: Could not open the database");
  }
  // Setup the GeoModel reader
  GeoModelIO::ReadGeoModel geoReader = GeoModelIO::ReadGeoModel(db);
  GeoModelTree geoModel{geoReader.buildGeoModel()};
  return geoModel;
}
