cmake_minimum_required(VERSION 3.14)
project(geoson VERSION 0.1.0 LANGUAGES CXX)

# -----------------------------------------------------------------------------
# 1) Create the header‐only INTERFACE library
# -----------------------------------------------------------------------------
add_library(geoson INTERFACE)

# Allow users to link via `geoson::geoson`
add_library(geoson::geoson ALIAS geoson)

# -----------------------------------------------------------------------------
# 2) Public include path
# -----------------------------------------------------------------------------
# This makes <geoson/enu_geosoner.h> etc. resolve to include/geoson/...
target_include_directories(geoson
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------------
# 3) Installation & Export for find_package()
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

# 3a) Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 3b) Install and export the INTERFACE target
install(TARGETS geoson EXPORT geosonTargets)

# 3c) Install the export file so that `find_package(geoson)` works
install(EXPORT geosonTargets
  FILE geosonTargets.cmake
  NAMESPACE geoson::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/geoson
)

# 3d) Also export it for in‐source Fetchgeoson usage
export(EXPORT geosonTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/geosonTargets.cmake"
  NAMESPACE geoson::
)

