# Add the python bindings for the DETRAY plugin
if(ACTS_BUILD_PLUGIN_TRACCC)
    target_link_libraries(ActsPythonBindings PUBLIC ActsPluginCovfie ActsPluginDetray)
    target_sources(ActsPythonBindings PRIVATE Covfie.cpp Detray.cpp)
else()
    target_sources(ActsPythonBindings PRIVATE CovfieStub.cpp DetrayStub.cpp)
endif()

# Add the python bindings for the DD4hep plugin
if(ACTS_BUILD_PLUGIN_DD4HEP)
    target_link_libraries(ActsPythonBindings PUBLIC ActsPluginDD4hep)
    target_sources(ActsPythonBindings PRIVATE DD4hepModule.cpp)
endif()


# Add the python bindings for the GeoModel plugin
if(ACTS_BUILD_PLUGIN_GEOMODEL)
    target_link_libraries(ActsPythonBindings PUBLIC ActsPluginGeoModel)
    target_sources(ActsPythonBindings PRIVATE GeoModel.cpp)
else()
    target_sources(ActsPythonBindings PRIVATE GeoModelStub.cpp)
endif()

# Add the python bindings for the JSON plugin
if(ACTS_BUILD_PLUGIN_JSON)
    target_link_libraries(ActsPythonBindings PUBLIC ActsPluginJson)
    target_sources(ActsPythonBindings PRIVATE Json.cpp)
else()
    target_sources(ActsPythonBindings PRIVATE JsonStub.cpp)
endif()

target_sources(ActsPythonBindings PRIVATE PluginsEntry.cpp)

# Add the python bindings for the SVG plugin
if(ACTS_BUILD_PLUGIN_ACTSVG)
    target_link_libraries(ActsPythonBindings PUBLIC ActsPluginActSVG)
    target_sources(ActsPythonBindings PRIVATE Svg.cpp)
else()
    target_sources(ActsPythonBindings PRIVATE SvgStub.cpp)
endif()

# Add the python bindings for the TGeo plugin
if(ACTS_BUILD_PLUGIN_TGEO)
    target_link_libraries(ActsPythonBindings PUBLIC ActsPluginTGeo)
    target_sources(ActsPythonBindings PRIVATE TGeo.cpp)
else()
    target_sources(ActsPythonBindings PRIVATE TGeoStub.cpp)
endif()

