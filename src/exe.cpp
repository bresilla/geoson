#include "concord/types_basic.hpp"
#include "geoson/parser.hpp"

#include <iostream>

int main() {

    concord::Datum datum{/*lat=*/52.0, /*lon=*/4.4, /*alt=*/0.0};

    auto fc = geoson::ReadFeatureCollection("misc/wur.geojson", datum);

    std::cout << fc.features.size() << " features\n" << std::endl;

    for (auto const &f : fc.features) {
        std::cout << "  properties: " << f.properties.size() << "\n";
        auto &v = f.geometry;
        if (auto *p = std::get_if<concord::Polygon>(&v)) {
            std::cout << "    Polygon\n";
        }
    }

    return 0;
}
