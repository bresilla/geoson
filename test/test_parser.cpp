#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

TEST_CASE("Parser - parseProperties") {
    SUBCASE("String properties") {
        nlohmann::json props = {{"name", "test_name"}, {"description", "test_description"}};

        auto result = geoson::parseProperties(props);

        CHECK(result.size() == 2);
        CHECK(result["name"] == "test_name");
        CHECK(result["description"] == "test_description");
    }

    SUBCASE("Mixed properties") {
        nlohmann::json props = {
            {"name", "test_name"}, {"number", 42}, {"boolean", true}, {"array", nlohmann::json::array({1, 2, 3})}};

        auto result = geoson::parseProperties(props);

        CHECK(result.size() == 4);
        CHECK(result["name"] == "test_name");
        CHECK(result["number"] == "42");
        CHECK(result["boolean"] == "true");
        CHECK(result["array"] == "[1,2,3]");
    }
}

TEST_CASE("Parser - parsePoint") {
    concord::Datum datum{52.0, 5.0, 0.0};

    SUBCASE("2D point") {
        nlohmann::json coords = {5.1, 52.1};

        auto point = geoson::parsePoint(coords, datum);

        CHECK(point.wgs.lon == doctest::Approx(5.1));
        CHECK(point.wgs.lat == doctest::Approx(52.1));
        CHECK(point.wgs.alt == doctest::Approx(0.0));
    }

    SUBCASE("3D point") {
        nlohmann::json coords = {5.1, 52.1, 10.0};

        auto point = geoson::parsePoint(coords, datum);

        CHECK(point.wgs.lon == doctest::Approx(5.1));
        CHECK(point.wgs.lat == doctest::Approx(52.1));
        CHECK(point.wgs.alt == doctest::Approx(10.0));
    }
}

TEST_CASE("Parser - parseLineString") {
    concord::Datum datum{52.0, 5.0, 0.0};

    SUBCASE("Two points (Line)") {
        nlohmann::json coords = {{5.1, 52.1, 0.0}, {5.2, 52.2, 0.0}};

        auto geom = geoson::parseLineString(coords, datum);

        CHECK(std::holds_alternative<concord::Line>(geom));
    }

    SUBCASE("Multiple points (Path)") {
        nlohmann::json coords = {{5.1, 52.1, 0.0}, {5.2, 52.2, 0.0}, {5.3, 52.3, 0.0}};

        auto geom = geoson::parseLineString(coords, datum);

        CHECK(std::holds_alternative<concord::Path>(geom));
    }
}

TEST_CASE("Parser - parsePolygon") {
    concord::Datum datum{52.0, 5.0, 0.0};

    nlohmann::json coords = {{
        {5.1, 52.1, 0.0}, {5.2, 52.1, 0.0}, {5.2, 52.2, 0.0}, {5.1, 52.2, 0.0}, {5.1, 52.1, 0.0} // closed ring
    }};

    auto polygon = geoson::parsePolygon(coords, datum);

    // Check that we get a polygon
    CHECK(polygon.getPoints().size() == 5);
}

TEST_CASE("Parser - parseGeometry") {
    concord::Datum datum{52.0, 5.0, 0.0};

    SUBCASE("Point geometry") {
        nlohmann::json geom = {{"type", "Point"}, {"coordinates", {5.1, 52.1, 0.0}}};

        auto geometries = geoson::parseGeometry(geom, datum);

        CHECK(geometries.size() == 1);
        CHECK(std::holds_alternative<concord::Point>(geometries[0]));
    }

    SUBCASE("LineString geometry") {
        nlohmann::json geom = {{"type", "LineString"}, {"coordinates", {{5.1, 52.1, 0.0}, {5.2, 52.2, 0.0}}}};

        auto geometries = geoson::parseGeometry(geom, datum);

        CHECK(geometries.size() == 1);
        CHECK(std::holds_alternative<concord::Line>(geometries[0]));
    }

    SUBCASE("Polygon geometry") {
        nlohmann::json geom = {
            {"type", "Polygon"},
            {"coordinates",
             {{{5.1, 52.1, 0.0}, {5.2, 52.1, 0.0}, {5.2, 52.2, 0.0}, {5.1, 52.2, 0.0}, {5.1, 52.1, 0.0}}}}};

        auto geometries = geoson::parseGeometry(geom, datum);

        CHECK(geometries.size() == 1);
        CHECK(std::holds_alternative<concord::Polygon>(geometries[0]));
    }

    SUBCASE("MultiPoint geometry") {
        nlohmann::json geom = {{"type", "MultiPoint"},
                               {"coordinates", {{5.1, 52.1, 0.0}, {5.2, 52.2, 0.0}, {5.3, 52.3, 0.0}}}};

        auto geometries = geoson::parseGeometry(geom, datum);

        CHECK(geometries.size() == 3);
        for (const auto &g : geometries) {
            CHECK(std::holds_alternative<concord::Point>(g));
        }
    }

    SUBCASE("GeometryCollection") {
        nlohmann::json geom = {{"type", "GeometryCollection"},
                               {"geometries",
                                {{{"type", "Point"}, {"coordinates", {5.1, 52.1, 0.0}}},
                                 {{"type", "LineString"}, {"coordinates", {{5.2, 52.2, 0.0}, {5.3, 52.3, 0.0}}}}}}};

        auto geometries = geoson::parseGeometry(geom, datum);

        CHECK(geometries.size() == 2);
        CHECK(std::holds_alternative<concord::Point>(geometries[0]));
        CHECK(std::holds_alternative<concord::Line>(geometries[1]));
    }
}

TEST_CASE("Parser - parseCRS") {
    SUBCASE("WGS84 variants") {
        CHECK(geoson::parseCRS("EPSG:4326") == concord::CRS::WGS);
        CHECK(geoson::parseCRS("WGS84") == concord::CRS::WGS);
        CHECK(geoson::parseCRS("WGS") == concord::CRS::WGS);
    }

    SUBCASE("ENU variants") {
        CHECK(geoson::parseCRS("ENU") == concord::CRS::ENU);
        CHECK(geoson::parseCRS("ECEF") == concord::CRS::ENU);
    }

    SUBCASE("Unknown CRS throws") {
        CHECK_THROWS_AS(geoson::parseCRS("UNKNOWN"), std::runtime_error);
        CHECK_THROWS_WITH(geoson::parseCRS("INVALID"), "Unknown CRS string: INVALID");
    }
}

TEST_CASE("Parser - File operations") {
    // Create a temporary test file
    const std::string test_file_content = R"({
        "type": "FeatureCollection",
        "properties": {
            "crs": "EPSG:4326",
            "datum": [52.0, 5.0, 0.0],
            "heading": 2.0
        },
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [5.1, 52.1, 10.0]
                },
                "properties": {
                    "name": "test_point"
                }
            }
        ]
    })";

    const std::filesystem::path test_file = "/tmp/test_geoson.geojson";

    SUBCASE("ReadFeatureCollection - valid file") {
        // Write test file
        std::ofstream ofs(test_file);
        ofs << test_file_content;
        ofs.close();

        auto fc = geoson::ReadFeatureCollection(test_file);

        CHECK(fc.crs == concord::CRS::WGS);
        CHECK(fc.datum.lat == doctest::Approx(52.0));
        CHECK(fc.datum.lon == doctest::Approx(5.0));
        CHECK(fc.datum.alt == doctest::Approx(0.0));
        CHECK(fc.heading.yaw == doctest::Approx(2.0));
        CHECK(fc.features.size() == 1);

        const auto &feature = fc.features[0];
        CHECK(std::holds_alternative<concord::Point>(feature.geometry));
        CHECK(feature.properties.at("name") == "test_point");

        // Cleanup
        std::filesystem::remove(test_file);
    }

    SUBCASE("ReadFeatureCollection - nonexistent file throws") {
        CHECK_THROWS_AS(geoson::ReadFeatureCollection("/nonexistent/file.geojson"), std::runtime_error);
    }

    SUBCASE("ReadFeatureCollection - missing properties throws") {
        const std::string invalid_content = R"({
            "type": "FeatureCollection",
            "features": []
        })";

        std::ofstream ofs(test_file);
        ofs << invalid_content;
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "missing top-level 'properties'");

        // Cleanup
        std::filesystem::remove(test_file);
    }
}
