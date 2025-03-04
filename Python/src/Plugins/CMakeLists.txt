# Add the python bindings for the DETRAY plugin
if(ACTS_BUILD_PLUGIN_TRACCC)
    # Covfie 
    pybind11_add_module(ActsPythonBindingsCovfie CovfieModule.cpp)
    target_link_libraries(ActsPythonBindingsCovfie PUBLIC ActsPythonUtilities ActsPluginCovfie)
    # Detray
    pybind11_add_module(ActsPythonBindingsDetray DetrayModule.cpp)
    target_link_libraries(ActsPythonBindingsDetray PUBLIC ActsPythonUtilities ActsPluginCovfie ActsPluginDetray)
endif()

# Add the python bindings for the DD4hep plugin
if(ACTS_BUILD_PLUGIN_DD4HEP)
    pybind11_add_module(ActsPythonBindingsDD4hep DD4hepModule.cpp)
    target_link_libraries(ActsPythonBindingsDD4hep PUBLIC ActsPythonUtilities ActsPluginDD4hep)
endif()

# Add the python bindings for the GeoModel plugin
if(ACTS_BUILD_PLUGIN_GEOMODEL)
    pybind11_add_module(ActsPythonBindingsGeoModel GeoModelModule.cpp)
    target_link_libraries(ActsPythonBindingsGeoModel PUBLIC ActsPythonUtilities ActsPluginGeoModel)
endif()

# Add the python bindings for the JSON plugin
if(ACTS_BUILD_PLUGIN_JSON)
    pybind11_add_module(ActsPythonBindingsJson JsonModule.cpp)
    target_link_libraries(ActsPythonBindingsJson PUBLIC ActsPythonUtilities ActsPluginJson)
endif()

# Add the python bindings for the SVG plugin
if(ACTS_BUILD_PLUGIN_ACTSVG)
    pybind11_add_module(ActsPythonBindingsActSVG SvgModule.cpp)
    target_link_libraries(ActsPythonBindingsActSVG PUBLIC ActsPythonUtilities ActsPluginActSVG)
endif()

# Add the python bindings for the TGeo plugin
if(ACTS_BUILD_PLUGIN_TGEO)
    pybind11_add_module(ActsPythonBindingsTGeo TGeoModule.cpp)
    target_link_libraries(ActsPythonBindingsTGeo PUBLIC ActsPythonUtilities ActsPluginTGeo)
endif()

