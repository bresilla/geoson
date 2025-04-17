cmake_minimum_required(VERSION 3.14)
project(geoson VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

# -----------------------------------------------------------------------------
# 1) Fetch nlohmann/json dependency
# -----------------------------------------------------------------------------
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f9e50780b8726b06865be2626edf1e3f385756f8297
)
FetchContent_MakeAvailable(json)

# -----------------------------------------------------------------------------
# 2) Create the header‐only INTERFACE library
# -----------------------------------------------------------------------------
add_library(geoson INTERFACE)

# Allow users to link via `geoson::geoson`
add_library(geoson::geoson ALIAS geoson)

# -----------------------------------------------------------------------------
# 3) Public include path and dependencies
# -----------------------------------------------------------------------------
# This makes <geoson/enu_geosoner.h> etc. resolve to include/geoson/...
target_include_directories(geoson
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link against nlohmann_json
target_link_libraries(geoson
    INTERFACE
        nlohmann_json::nlohmann_json
)

# -----------------------------------------------------------------------------
# 4) Installation & Export for find_package()
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

# 4a) Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 4b) Install and export the INTERFACE target
install(TARGETS geoson nlohmann_json
    EXPORT geosonTargets
)

# 4c) Install the export file so that `find_package(geoson)` works
install(EXPORT geosonTargets
    FILE geosonTargets.cmake
    NAMESPACE geoson::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/geoson
)

# 4d) Also export it for in‐source FetchContent usage
export(EXPORT geosonTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/geosonTargets.cmake"
    NAMESPACE geoson::
)
