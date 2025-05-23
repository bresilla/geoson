#pragma once

#include "concord/types_basic.hpp" // for concord::CRS, Datum, Euler
#include "concord/types_line.hpp"
#include "concord/types_path.hpp"
#include "concord/types_polygon.hpp"

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace geoson {
    using Geometry = std::variant<concord::Point, concord::Line, concord::Path, concord::Polygon>;

    struct Feature {
        Geometry geometry;
        std::unordered_map<std::string, std::string> properties;
    };

    struct FeatureCollection {
        concord::CRS crs;
        concord::Datum datum;
        concord::Euler heading;
        std::vector<Feature> features;
    };

} // namespace geoson
