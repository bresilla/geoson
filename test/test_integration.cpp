#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

TEST_CASE("Integration - Round-trip conversion") {
    // Create original feature collection
    concord::CRS crs = concord::CRS::WGS;
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    std::vector<geoson::Feature> features;

    // Add various geometry types
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    pointProps["category"] = "landmark";
    features.emplace_back(geoson::Feature{point, pointProps});

    concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};
    concord::Point end{concord::WGS{52.2, 5.2, 0.0}, datum};
    concord::Line line{start, end};
    std::unordered_map<std::string, std::string> lineProps;
    lineProps["name"] = "test_line";
    features.emplace_back(geoson::Feature{line, lineProps});

    std::vector<concord::Point> pathPoints = {concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},
                                              concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum},
                                              concord::Point{concord::WGS{52.3, 5.3, 0.0}, datum}};
    concord::Path path{pathPoints};
    std::unordered_map<std::string, std::string> pathProps;
    pathProps["name"] = "test_path";
    features.emplace_back(geoson::Feature{path, pathProps});

    std::vector<concord::Point> polygonPoints = {
        concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}, concord::Point{concord::WGS{52.2, 5.1, 0.0}, datum},
        concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum}, concord::Point{concord::WGS{52.1, 5.2, 0.0}, datum},
        concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}};
    concord::Polygon polygon{polygonPoints};
    std::unordered_map<std::string, std::string> polygonProps;
    polygonProps["name"] = "test_polygon";
    features.emplace_back(geoson::Feature{polygon, polygonProps});

    geoson::FeatureCollection original{crs, datum, heading, std::move(features)};

    const std::filesystem::path test_file = "/tmp/test_roundtrip.geojson";

    // Write to file
    geoson::WriteFeatureCollection(original, test_file);

    // Read back
    auto restored = geoson::ReadFeatureCollection(test_file);

    // Compare
    CHECK(restored.crs == original.crs);
    CHECK(restored.datum.lat == doctest::Approx(original.datum.lat));
    CHECK(restored.datum.lon == doctest::Approx(original.datum.lon));
    CHECK(restored.datum.alt == doctest::Approx(original.datum.alt));
    CHECK(restored.heading.yaw == doctest::Approx(original.heading.yaw));
    CHECK(restored.features.size() == original.features.size());

    // Check individual features
    for (size_t i = 0; i < restored.features.size(); ++i) {
        const auto &origFeature = original.features[i];
        const auto &restoredFeature = restored.features[i];

        // Check geometry types match
        CHECK(origFeature.geometry.index() == restoredFeature.geometry.index());

        // Check properties
        for (const auto &[key, value] : origFeature.properties) {
            CHECK(restoredFeature.properties.count(key) > 0);
            CHECK(restoredFeature.properties.at(key) == value);
        }
    }

    // Cleanup
    std::filesystem::remove(test_file);
}

TEST_CASE("Integration - Real GeoJSON file parsing") {
    // Test with a real-world-like GeoJSON structure
    const std::string realistic_geojson = R"({
        "type": "FeatureCollection",
        "properties": {
            "crs": "EPSG:4326",
            "datum": [51.98764, 5.660062, 0.0],
            "heading": 0.0
        },
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "Polygon",
                    "coordinates": [[
                        [5.660062043558668, 51.98764028186088, 0.0],
                        [5.6618289715088395, 51.988126870487235, 0.0],
                        [5.661049882650161, 51.98908317675762, 0.0],
                        [5.66289230646484, 51.98958409291862, 0.0],
                        [5.662003964010751, 51.99056338815885, 0.0],
                        [5.658587856677201, 51.989514414720105, 0.0],
                        [5.660062043558668, 51.98764028186088, 0.0]
                    ]]
                },
                "properties": {
                    "name": "Field 4",
                    "area": "agricultural",
                    "crop": "wheat"
                }
            },
            {
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [5.660062, 51.98764, 15.0]
                },
                "properties": {
                    "name": "Farm Center",
                    "type": "building"
                }
            }
        ]
    })";

    const std::filesystem::path test_file = "/tmp/realistic_test.geojson";

    // Write test file
    std::ofstream ofs(test_file);
    ofs << realistic_geojson;
    ofs.close();

    // Parse the file
    auto fc = geoson::ReadFeatureCollection(test_file);

    // Verify parsed content
    CHECK(fc.crs == concord::CRS::WGS);
    CHECK(fc.datum.lat == doctest::Approx(51.98764));
    CHECK(fc.datum.lon == doctest::Approx(5.660062));
    CHECK(fc.datum.alt == doctest::Approx(0.0));
    CHECK(fc.heading.yaw == doctest::Approx(0.0));
    CHECK(fc.features.size() == 2);

    // Check polygon feature
    const auto &polygonFeature = fc.features[0];
    CHECK(std::holds_alternative<concord::Polygon>(polygonFeature.geometry));
    CHECK(polygonFeature.properties.at("name") == "Field 4");
    CHECK(polygonFeature.properties.at("area") == "agricultural");
    CHECK(polygonFeature.properties.at("crop") == "wheat");

    // Check point feature
    const auto &pointFeature = fc.features[1];
    CHECK(std::holds_alternative<concord::Point>(pointFeature.geometry));
    CHECK(pointFeature.properties.at("name") == "Farm Center");
    CHECK(pointFeature.properties.at("type") == "building");

    // Test the point coordinates
    auto &point = std::get<concord::Point>(pointFeature.geometry);
    CHECK(point.wgs.lon == doctest::Approx(5.660062));
    CHECK(point.wgs.lat == doctest::Approx(51.98764));
    CHECK(point.wgs.alt == doctest::Approx(15.0));

    // Cleanup
    std::filesystem::remove(test_file);
}

TEST_CASE("Integration - Feature wrapping") {
    // Test single Feature gets wrapped into FeatureCollection
    const std::string single_feature = R"({
        "type": "Feature",
        "geometry": {
            "type": "Point",
            "coordinates": [5.1, 52.1, 0.0]
        },
        "properties": {
            "name": "single_point"
        }
    })";

    const std::filesystem::path test_file = "/tmp/single_feature.geojson";

    // Write test file
    std::ofstream ofs(test_file);
    ofs << single_feature;
    ofs.close();

    // This should throw because single features don't have top-level properties
    CHECK_THROWS_AS(geoson::ReadFeatureCollection(test_file), std::runtime_error);

    // Cleanup
    std::filesystem::remove(test_file);
}

TEST_CASE("Integration - Geometry wrapping") {
    // Test bare geometry gets wrapped
    const std::string bare_geometry = R"({
        "type": "Point",
        "coordinates": [5.1, 52.1, 0.0]
    })";

    const std::filesystem::path test_file = "/tmp/bare_geometry.geojson";

    // Write test file
    std::ofstream ofs(test_file);
    ofs << bare_geometry;
    ofs.close();

    // This should also throw because bare geometries don't have top-level properties
    CHECK_THROWS_AS(geoson::ReadFeatureCollection(test_file), std::runtime_error);

    // Cleanup
    std::filesystem::remove(test_file);
}

TEST_CASE("Integration - Pretty printing") {
    concord::CRS crs = concord::CRS::WGS;
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    std::vector<geoson::Feature> features;

    // Add one of each geometry type
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, pointProps});

    concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};
    concord::Point end{concord::WGS{52.2, 5.2, 0.0}, datum};
    concord::Line line{start, end};
    std::unordered_map<std::string, std::string> lineProps;
    features.emplace_back(geoson::Feature{line, lineProps});

    std::vector<concord::Point> pathPoints = {concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},
                                              concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum},
                                              concord::Point{concord::WGS{52.3, 5.3, 0.0}, datum}};
    concord::Path path{pathPoints};
    std::unordered_map<std::string, std::string> pathProps;
    features.emplace_back(geoson::Feature{path, pathProps});

    std::vector<concord::Point> polygonPoints = {
        concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}, concord::Point{concord::WGS{52.2, 5.1, 0.0}, datum},
        concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum}, concord::Point{concord::WGS{52.1, 5.2, 0.0}, datum},
        concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}};
    concord::Polygon polygon{polygonPoints};
    std::unordered_map<std::string, std::string> polygonProps;
    features.emplace_back(geoson::Feature{polygon, polygonProps});

    geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

    // Test the pretty print operator
    std::ostringstream oss;
    oss << fc;
    std::string output = oss.str();

    // Check that expected content is in the output
    CHECK(output.find("CRS: WGS") != std::string::npos);
    CHECK(output.find("DATUM: 52, 5, 0") != std::string::npos);
    CHECK(output.find("HEADING: 2") != std::string::npos);
    CHECK(output.find("FEATURES: 4") != std::string::npos);
    CHECK(output.find("POINT") != std::string::npos);
    CHECK(output.find("LINE") != std::string::npos);
    CHECK(output.find("PATH") != std::string::npos);
    CHECK(output.find("POLYGON") != std::string::npos);
    CHECK(output.find("PROPS:1") != std::string::npos); // point has 1 property
}
