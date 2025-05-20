#include "concord/types_basic.hpp"
#include "geoson/parser.hpp"

int main() {

    concord::Datum datum{/*lat=*/52.0, /*lon=*/4.4, /*alt=*/0.0};

    auto fc = geoson::ReadFeatureCollection("../misc/field4.geojson", datum);

    return 0;
}
