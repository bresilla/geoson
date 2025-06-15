#pragma once

#include "concord/concord.hpp" // for Datum, Euler, geometric types

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace geoson {
    using Geometry = std::variant<concord::Point, concord::Line, concord::Path, concord::Polygon>;

    // Simple CRS representation since concord::CRS no longer exists
    enum class CRS { WGS, ENU };

    struct Feature {
        Geometry geometry;
        std::unordered_map<std::string, std::string> properties;
    };

    struct FeatureCollection {
        CRS crs;
        concord::Datum datum;
        concord::Euler heading;
        std::vector<Feature> features;
    };

} // namespace geoson
