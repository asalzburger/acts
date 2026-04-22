// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsPlugins/Json/IVolumeMaterialJsonDecorator.hpp"
#include "ActsPlugins/Json/MaterialMapJsonConverter.hpp"
#include "ActsTests/CommonHelpers/DataDirectory.hpp"

#include <fstream>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace Acts {
class IVolumeMaterial;
}  // namespace Acts

using namespace Acts;

class DummyDecorator : public IVolumeMaterialJsonDecorator {
 public:
  void decorate([[maybe_unused]] const ISurfaceMaterial& material,
                [[maybe_unused]] nlohmann::json& json) const override {};

  void decorate([[maybe_unused]] const IVolumeMaterial& material,
                [[maybe_unused]] nlohmann::json& json) const override {};
};

namespace ActsTests {

BOOST_AUTO_TEST_SUITE(JsonSuite)

namespace {
void checkSurfaceMaterialSchema(const nlohmann::json& node) {
  if (!node.is_object()) {
    return;
  }

  if (node.contains("material") && node["material"].is_object()) {
    const auto& material = node["material"];
    if (material.contains("type")) {
      const std::string type = material["type"];
      if (type == "proto" || type == "binned") {
        BOOST_CHECK(material.contains("directedProtoAxes"));
        BOOST_CHECK(!material.contains("binUtility"));
      }
    }
  }

  for (const auto& [key, value] : node.items()) {
    (void)key;
    if (value.is_array()) {
      for (const auto& entry : value) {
        checkSurfaceMaterialSchema(entry);
      }
    } else {
      checkSurfaceMaterialSchema(value);
    }
  }
}
}  // namespace

BOOST_AUTO_TEST_CASE(RoundtripFromFile) {
  // read reference map from file
  std::ifstream refFile(ActsTests::getDataPath("material-map.json"));
  nlohmann::json refJson;
  refFile >> refJson;

  DummyDecorator decorator;
  // convert to the material map and back again
  MaterialMapJsonConverter::Config converterCfg;
  MaterialMapJsonConverter converter(converterCfg, Logging::INFO);
  auto materialMap = converter.jsonToMaterialMaps(refJson);
  nlohmann::json encodedJson =
      converter.materialMapsToJson(materialMap, &decorator);

  // legacy binUtility JSON is accepted as input, output now uses
  // directedProtoAxes
  BOOST_CHECK(encodedJson.contains("Surfaces"));
  BOOST_CHECK(encodedJson.contains("Volumes"));
  checkSurfaceMaterialSchema(encodedJson["Surfaces"]);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ActsTests
