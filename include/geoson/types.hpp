#pragma once

#include "concord/concord.hpp" // for Datum, Euler, geometric types

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace geoson {
    // Internal geometry representation: all coordinates are stored as Point (ENU/local system)
    // Regardless of input CRS, coordinates are converted to local coordinate system during parsing
    using Geometry = std::variant<concord::Point, concord::Line, concord::Path, concord::Polygon>;

    // Simple CRS representation - used for input parsing and output formatting
    enum class CRS { WGS, ENU };

    struct Feature {
        Geometry geometry;
        std::unordered_map<std::string, std::string> properties;
    };

    struct FeatureCollection {
        CRS crs; // Original CRS from input file (for reference/default output)
        concord::Datum datum;
        concord::Euler heading;
        std::vector<Feature> features; // All geometries stored in Point (ENU/local) coordinates
    };

} // namespace geoson
