#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <variant>

TEST_CASE("Types - Basic Geometry Variant") {
    SUBCASE("Point geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};

        geoson::Geometry geom = point;

        CHECK(std::holds_alternative<concord::Point>(geom));
        auto *p = std::get_if<concord::Point>(&geom);
        REQUIRE(p != nullptr);
        CHECK(p->wgs.lat == doctest::Approx(52.1));
        CHECK(p->wgs.lon == doctest::Approx(5.1));
        CHECK(p->wgs.alt == doctest::Approx(10.0));
    }

    SUBCASE("Line geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};
        concord::Point end{concord::WGS{52.2, 5.2, 0.0}, datum};
        concord::Line line{start, end};

        geoson::Geometry geom = line;

        CHECK(std::holds_alternative<concord::Line>(geom));
    }

    SUBCASE("Path geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        std::vector<concord::Point> points = {concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},
                                              concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum},
                                              concord::Point{concord::WGS{52.3, 5.3, 0.0}, datum}};
        concord::Path path{points};

        geoson::Geometry geom = path;

        CHECK(std::holds_alternative<concord::Path>(geom));
    }

    SUBCASE("Polygon geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        std::vector<concord::Point> points = {
            concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}, concord::Point{concord::WGS{52.2, 5.1, 0.0}, datum},
            concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum}, concord::Point{concord::WGS{52.1, 5.2, 0.0}, datum},
            concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}};
        concord::Polygon polygon{points};

        geoson::Geometry geom = polygon;

        CHECK(std::holds_alternative<concord::Polygon>(geom));
    }
}

TEST_CASE("Types - Feature") {
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};

    std::unordered_map<std::string, std::string> properties;
    properties["name"] = "test_feature";
    properties["type"] = "point_of_interest";

    geoson::Feature feature{point, properties};

    CHECK(std::holds_alternative<concord::Point>(feature.geometry));
    CHECK(feature.properties.size() == 2);
    CHECK(feature.properties["name"] == "test_feature");
    CHECK(feature.properties["type"] == "point_of_interest");
}

TEST_CASE("Types - FeatureCollection") {
    concord::CRS crs = concord::CRS::WGS;
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    // Create some features
    std::vector<geoson::Feature> features;

    // Add a point feature
    concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, pointProps});

    // Add a line feature
    concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};
    concord::Point end{concord::WGS{52.2, 5.2, 0.0}, datum};
    concord::Line line{start, end};
    std::unordered_map<std::string, std::string> lineProps;
    lineProps["name"] = "test_line";
    features.emplace_back(geoson::Feature{line, lineProps});

    geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

    CHECK(fc.crs == concord::CRS::WGS);
    CHECK(fc.datum.lat == doctest::Approx(52.0));
    CHECK(fc.datum.lon == doctest::Approx(5.0));
    CHECK(fc.datum.alt == doctest::Approx(0.0));
    CHECK(fc.heading.yaw == doctest::Approx(2.0));
    CHECK(fc.features.size() == 2);

    // Check first feature (point)
    CHECK(std::holds_alternative<concord::Point>(fc.features[0].geometry));
    CHECK(fc.features[0].properties["name"] == "test_point");

    // Check second feature (line)
    CHECK(std::holds_alternative<concord::Line>(fc.features[1].geometry));
    CHECK(fc.features[1].properties["name"] == "test_line");
}
