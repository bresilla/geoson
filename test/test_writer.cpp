#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

TEST_CASE("Writer - geometryToJson") {
    concord::Datum datum{52.0, 5.0, 0.0};

    SUBCASE("Point to JSON") {
        concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
        geoson::Geometry geom = point;

        auto json = geoson::geometryToJson(geom);

        CHECK(json["type"] == "Point");
        CHECK(json["coordinates"].is_array());
        CHECK(json["coordinates"].size() == 3);
        CHECK(json["coordinates"][0] == doctest::Approx(5.1));  // lon
        CHECK(json["coordinates"][1] == doctest::Approx(52.1)); // lat
        CHECK(json["coordinates"][2] == doctest::Approx(10.0)); // alt
    }

    SUBCASE("Line to JSON") {
        concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};
        concord::Point end{concord::WGS{52.2, 5.2, 0.0}, datum};
        concord::Line line{start, end};
        geoson::Geometry geom = line;

        auto json = geoson::geometryToJson(geom);

        CHECK(json["type"] == "LineString");
        CHECK(json["coordinates"].is_array());
        CHECK(json["coordinates"].size() == 2);

        // Check start point
        CHECK(json["coordinates"][0][0] == doctest::Approx(5.1));
        CHECK(json["coordinates"][0][1] == doctest::Approx(52.1));
        CHECK(json["coordinates"][0][2] == doctest::Approx(0.0));

        // Check end point
        CHECK(json["coordinates"][1][0] == doctest::Approx(5.2));
        CHECK(json["coordinates"][1][1] == doctest::Approx(52.2));
        CHECK(json["coordinates"][1][2] == doctest::Approx(0.0));
    }

    SUBCASE("Path to JSON") {
        std::vector<concord::Point> points = {concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},
                                              concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum},
                                              concord::Point{concord::WGS{52.3, 5.3, 0.0}, datum}};
        concord::Path path{points};
        geoson::Geometry geom = path;

        auto json = geoson::geometryToJson(geom);

        CHECK(json["type"] == "LineString");
        CHECK(json["coordinates"].is_array());
        CHECK(json["coordinates"].size() == 3);

        // Check coordinates
        for (int i = 0; i < 3; i++) {
            CHECK(json["coordinates"][i][0] == doctest::Approx(5.1 + i * 0.1));
            CHECK(json["coordinates"][i][1] == doctest::Approx(52.1 + i * 0.1));
            CHECK(json["coordinates"][i][2] == doctest::Approx(0.0));
        }
    }

    SUBCASE("Polygon to JSON") {
        std::vector<concord::Point> points = {
            concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}, concord::Point{concord::WGS{52.2, 5.1, 0.0}, datum},
            concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum}, concord::Point{concord::WGS{52.1, 5.2, 0.0}, datum},
            concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum} // closed ring
        };
        concord::Polygon polygon{points};
        geoson::Geometry geom = polygon;

        auto json = geoson::geometryToJson(geom);

        CHECK(json["type"] == "Polygon");
        CHECK(json["coordinates"].is_array());
        CHECK(json["coordinates"].size() == 1);    // one ring
        CHECK(json["coordinates"][0].size() == 5); // 5 points in ring
    }
}

TEST_CASE("Writer - featureToJson") {
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};

    std::unordered_map<std::string, std::string> properties;
    properties["name"] = "test_feature";
    properties["type"] = "landmark";

    geoson::Feature feature{point, properties};

    auto json = geoson::featureToJson(feature);

    CHECK(json["type"] == "Feature");
    CHECK(json.contains("geometry"));
    CHECK(json.contains("properties"));

    // Check geometry
    CHECK(json["geometry"]["type"] == "Point");

    // Check properties
    CHECK(json["properties"]["name"] == "test_feature");
    CHECK(json["properties"]["type"] == "landmark");
}

TEST_CASE("Writer - toJson FeatureCollection") {
    concord::CRS crs = concord::CRS::WGS;
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    // Create features
    std::vector<geoson::Feature> features;

    // Point feature
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, pointProps});

    // Line feature
    concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};
    concord::Point end{concord::WGS{52.2, 5.2, 0.0}, datum};
    concord::Line line{start, end};
    std::unordered_map<std::string, std::string> lineProps;
    lineProps["name"] = "test_line";
    features.emplace_back(geoson::Feature{line, lineProps});

    geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

    auto json = geoson::toJson(fc);

    SUBCASE("Top-level structure") {
        CHECK(json["type"] == "FeatureCollection");
        CHECK(json.contains("properties"));
        CHECK(json.contains("features"));
    }

    SUBCASE("Properties") {
        auto &props = json["properties"];
        CHECK(props["crs"] == "EPSG:4326");
        CHECK(props["datum"].is_array());
        CHECK(props["datum"].size() == 3);
        CHECK(props["datum"][0] == doctest::Approx(52.0));
        CHECK(props["datum"][1] == doctest::Approx(5.0));
        CHECK(props["datum"][2] == doctest::Approx(0.0));
        CHECK(props["heading"] == doctest::Approx(2.0));
    }

    SUBCASE("Features") {
        CHECK(json["features"].is_array());
        CHECK(json["features"].size() == 2);

        // Check point feature
        auto &pointFeature = json["features"][0];
        CHECK(pointFeature["type"] == "Feature");
        CHECK(pointFeature["geometry"]["type"] == "Point");
        CHECK(pointFeature["properties"]["name"] == "test_point");

        // Check line feature
        auto &lineFeature = json["features"][1];
        CHECK(lineFeature["type"] == "Feature");
        CHECK(lineFeature["geometry"]["type"] == "LineString");
        CHECK(lineFeature["properties"]["name"] == "test_line");
    }
}

TEST_CASE("Writer - toJson with ENU CRS") {
    concord::CRS crs = concord::CRS::ENU;
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 1.5};

    std::vector<geoson::Feature> features;
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
    std::unordered_map<std::string, std::string> props;
    features.emplace_back(geoson::Feature{point, props});

    geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

    auto json = geoson::toJson(fc);

    CHECK(json["properties"]["crs"] == "ENU");
}

TEST_CASE("Writer - WriteFeatureCollection") {
    concord::CRS crs = concord::CRS::WGS;
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    std::vector<geoson::Feature> features;
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
    std::unordered_map<std::string, std::string> props;
    props["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, props});

    geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

    const std::filesystem::path test_file = "/tmp/test_output.geojson";

    SUBCASE("Write and read back") {
        geoson::WriteFeatureCollection(fc, test_file);

        // Verify file exists
        CHECK(std::filesystem::exists(test_file));

        // Read back and verify content
        std::ifstream ifs(test_file);
        nlohmann::json json;
        ifs >> json;
        ifs.close();

        CHECK(json["type"] == "FeatureCollection");
        CHECK(json["properties"]["crs"] == "EPSG:4326");
        CHECK(json["features"].size() == 1);
        CHECK(json["features"][0]["properties"]["name"] == "test_point");

        // Cleanup
        std::filesystem::remove(test_file);
    }

    SUBCASE("Write to invalid path throws") {
        CHECK_THROWS_AS(geoson::WriteFeatureCollection(fc, "/invalid/path/file.geojson"), std::runtime_error);
    }
}
