#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

TEST_CASE("Error Handling - Invalid JSON") {
    const std::filesystem::path test_file = "/tmp/invalid.geojson";

    SUBCASE("Malformed JSON") {
        std::ofstream ofs(test_file);
        ofs << "{ invalid json content }";
        ofs.close();

        CHECK_THROWS_AS(geoson::ReadFeatureCollection(test_file), nlohmann::json::parse_error);

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing type field") {
        std::ofstream ofs(test_file);
        ofs << R"({"features": []})";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file),
                          "geoson::ReadFeatureCollection(): top-level object has no string 'type' field");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Non-string type field") {
        std::ofstream ofs(test_file);
        ofs << R"({"type": 123, "features": []})";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file),
                          "geoson::ReadFeatureCollection(): top-level object has no string 'type' field");

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Error Handling - Missing required properties") {
    const std::filesystem::path test_file = "/tmp/missing_props.geojson";

    SUBCASE("Missing properties object") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "missing top-level 'properties'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Properties not an object") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": "invalid",
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "missing top-level 'properties'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing CRS") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "datum": [52.0, 5.0, 0.0],
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "'properties' missing string 'crs'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Non-string CRS") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": 123,
                "datum": [52.0, 5.0, 0.0],
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "'properties' missing string 'crs'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing datum") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "'properties' missing array 'datum' of ≥3 numbers");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Invalid datum - not array") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": "invalid",
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "'properties' missing array 'datum' of ≥3 numbers");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Invalid datum - too few elements") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [52.0, 5.0],
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "'properties' missing array 'datum' of ≥3 numbers");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing heading") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [52.0, 5.0, 0.0]
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "'properties' missing numeric 'heading'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Non-numeric heading") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [52.0, 5.0, 0.0],
                "heading": "invalid"
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(geoson::ReadFeatureCollection(test_file), "'properties' missing numeric 'heading'");

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Error Handling - Invalid geometry parsing") {
    concord::Datum datum{52.0, 5.0, 0.0};

    SUBCASE("Invalid point coordinates - too few") {
        nlohmann::json coords = {5.1}; // only longitude

        CHECK_THROWS_AS(geoson::parsePoint(coords, datum), nlohmann::json::out_of_range);
    }

    SUBCASE("Invalid point coordinates - non-numeric") {
        nlohmann::json coords = {"invalid", 52.1};

        CHECK_THROWS_AS(geoson::parsePoint(coords, datum), nlohmann::json::type_error);
    }

    SUBCASE("Invalid polygon coordinates - missing outer ring") {
        nlohmann::json coords = nlohmann::json::array(); // empty array

        CHECK_THROWS_AS(geoson::parsePolygon(coords, datum), nlohmann::json::out_of_range);
    }
}

TEST_CASE("Error Handling - File I/O errors") {
    SUBCASE("ReadFeatureCollection - nonexistent file") {
        CHECK_THROWS_WITH(geoson::ReadFeatureCollection("/nonexistent/path/file.geojson"),
                          doctest::Contains("geoson::ReadFeatureCollection(): cannot open"));
    }

    SUBCASE("WriteFeatureCollection - invalid directory") {
        concord::CRS crs = concord::CRS::WGS;
        concord::Datum datum{52.0, 5.0, 0.0};
        concord::Euler heading{0.0, 0.0, 0.0};
        std::vector<geoson::Feature> features;

        geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

        CHECK_THROWS_WITH(geoson::WriteFeatureCollection(fc, "/nonexistent/directory/file.geojson"),
                          doctest::Contains("Cannot open for write"));
    }
}

TEST_CASE("Error Handling - Unknown CRS") {
    SUBCASE("Completely unknown CRS") {
        CHECK_THROWS_WITH(geoson::parseCRS("UNKNOWN:12345"), "Unknown CRS string: UNKNOWN:12345");
    }

    SUBCASE("Empty CRS string") { CHECK_THROWS_WITH(geoson::parseCRS(""), "Unknown CRS string: "); }

    SUBCASE("Case sensitivity") {
        CHECK_THROWS_WITH(geoson::parseCRS("epsg:4326"), "Unknown CRS string: epsg:4326");
        CHECK_THROWS_WITH(geoson::parseCRS("wgs84"), "Unknown CRS string: wgs84");
        CHECK_THROWS_WITH(geoson::parseCRS("enu"), "Unknown CRS string: enu");
    }
}

TEST_CASE("Error Handling - Robust parsing") {
    const std::filesystem::path test_file = "/tmp/robust_test.geojson";

    SUBCASE("Features with null geometry are skipped") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [52.0, 5.0, 0.0],
                "heading": 0.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": null,
                    "properties": {"name": "null_geom"}
                },
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [5.1, 52.1, 0.0]
                    },
                    "properties": {"name": "valid_point"}
                }
            ]
        })";
        ofs.close();

        auto fc = geoson::ReadFeatureCollection(test_file);

        // Should only have one feature (the null geometry one is skipped)
        CHECK(fc.features.size() == 1);
        CHECK(fc.features[0].properties.at("name") == "valid_point");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing properties in feature defaults to empty") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [52.0, 5.0, 0.0],
                "heading": 0.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [5.1, 52.1, 0.0]
                    }
                }
            ]
        })";
        ofs.close();

        auto fc = geoson::ReadFeatureCollection(test_file);

        CHECK(fc.features.size() == 1);
        CHECK(fc.features[0].properties.empty());

        std::filesystem::remove(test_file);
    }
}
