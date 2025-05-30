#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>

#include "concord/concord.hpp" // for concord::CRS, Datum, Euler

#include "geoson/types.hpp"

namespace geoson {

    namespace op {
        inline nlohmann::json ReadFeatureCollection(const std::filesystem::path &file) {
            std::ifstream ifs(file);
            if (!ifs) {
                throw std::runtime_error("geoson::ReadFeatureCollection(): cannot open \"" + file.string() + '\"');
            }

            nlohmann::json j;
            ifs >> j;

            if (!j.is_object() || !j.contains("type") || !j["type"].is_string()) {
                throw std::runtime_error(
                    "geoson::ReadFeatureCollection(): top-level object has no string 'type' field");
            }

            auto type = j["type"].get<std::string>();
            if (type == "FeatureCollection") {
                return j;
            }
            if (type == "Feature") {
                return nlohmann::json{{"type", "FeatureCollection"}, {"features", nlohmann::json::array({j})}};
            }

            // bare geometry → wrap into a one-feature collection
            nlohmann::json feat = {{"type", "Feature"}, {"geometry", j}, {"properties", nlohmann::json::object()}};
            return nlohmann::json{{"type", "FeatureCollection"}, {"features", nlohmann::json::array({feat})}};
        }
    } // namespace op

    using json = nlohmann::json;

    inline std::unordered_map<std::string, std::string> parseProperties(const json &props) {
        std::unordered_map<std::string, std::string> m;
        m.reserve(props.size());
        for (auto const &item : props.items()) {
            if (item.value().is_string())
                m[item.key()] = item.value().get<std::string>();
            else
                m[item.key()] = item.value().dump();
        }
        return m;
    }

    inline concord::Point parsePoint(const json &coords, const concord::Datum &datum) {
        double lon = coords.at(0).get<double>();
        double lat = coords.at(1).get<double>();
        double alt = coords.size() > 2 ? coords.at(2).get<double>() : 0.0;
        return concord::Point{concord::WGS{lat, lon, alt}, datum};
    }

    inline Geometry parseLineString(const json &coords, const concord::Datum &datum) {
        std::vector<concord::Point> pts;
        pts.reserve(coords.size());
        for (auto const &c : coords)
            pts.push_back(parsePoint(c, datum));
        if (pts.size() == 2)
            return concord::Line{pts[0], pts[1]};
        else
            return concord::Path{pts};
    }

    inline concord::Polygon parsePolygon(const json &coords, const concord::Datum &datum) {
        std::vector<concord::Point> pts;
        auto const &ring = coords.at(0);
        pts.reserve(ring.size());
        for (auto const &c : ring)
            pts.push_back(parsePoint(c, datum));
        return concord::Polygon{pts};
    }

    inline std::vector<Geometry> parseGeometry(const json &geom, const concord::Datum &datum) {
        std::vector<Geometry> out;
        auto type = geom.at("type").get<std::string>();

        if (type == "Point") {
            out.emplace_back(parsePoint(geom.at("coordinates"), datum));
        } else if (type == "LineString") {
            out.emplace_back(parseLineString(geom.at("coordinates"), datum));
        } else if (type == "Polygon") {
            out.emplace_back(parsePolygon(geom.at("coordinates"), datum));
        } else if (type == "MultiPoint") {
            for (auto const &c : geom.at("coordinates"))
                out.emplace_back(parsePoint(c, datum));
        } else if (type == "MultiLineString") {
            for (auto const &line : geom.at("coordinates"))
                out.emplace_back(parseLineString(line, datum));
        } else if (type == "MultiPolygon") {
            for (auto const &poly : geom.at("coordinates"))
                out.emplace_back(parsePolygon(poly, datum));
        } else if (type == "GeometryCollection") {
            for (auto const &sub : geom.at("geometries")) {
                auto subs = parseGeometry(sub, datum);
                out.insert(out.end(), subs.begin(), subs.end());
            }
        }
        return out;
    }

    // ––– parse the CRS string into the enum –––

    inline concord::CRS parseCRS(const std::string &s) {
        if (s == "EPSG:4326" || s == "WGS84" || s == "WGS")
            return concord::CRS::WGS;
        else if (s == "ENU" || s == "ECEF")
            return concord::CRS::ENU;
        throw std::runtime_error("Unknown CRS string: " + s);
    }

    // ––– main loader –––

    inline FeatureCollection ReadFeatureCollection(const std::filesystem::path &file) {
        auto fc_json = op::ReadFeatureCollection(file);

        if (!fc_json.contains("properties") || !fc_json["properties"].is_object())
            throw std::runtime_error("missing top-level 'properties'");
        auto &P = fc_json["properties"];

        if (!P.contains("crs") || !P["crs"].is_string())
            throw std::runtime_error("'properties' missing string 'crs'");
        if (!P.contains("datum") || !P["datum"].is_array() || P["datum"].size() < 3)
            throw std::runtime_error("'properties' missing array 'datum' of ≥3 numbers");
        if (!P.contains("heading") || !P["heading"].is_number())
            throw std::runtime_error("'properties' missing numeric 'heading'");

        // extract the new types
        auto crsVal = parseCRS(P["crs"].get<std::string>());
        auto &A = P["datum"];
        concord::Datum d{A[0].get<double>(), A[1].get<double>(), A[2].get<double>()};
        double yaw = P["heading"].get<double>();
        concord::Euler euler{0.0, 0.0, yaw};

        FeatureCollection fc;
        fc.crs = crsVal;
        fc.datum = d;
        fc.heading = euler;
        fc.features.reserve(fc_json["features"].size());

        for (auto const &feat : fc_json["features"]) {
            if (feat.value("geometry", json{}).is_null())
                continue;
            auto geoms = parseGeometry(feat["geometry"], d);
            auto props_map = parseProperties(feat.value("properties", json::object()));
            for (auto &g : geoms)
                fc.features.emplace_back(Feature{std::move(g), props_map});
        }

        return fc;
    }

    // ––– pretty-print FeatureCollection header –––

    inline std::ostream &operator<<(std::ostream &os, FeatureCollection const &fc) {
        os << "CRS: ";
        switch (fc.crs) {
        case concord::CRS::WGS:
            os << "WGS";
            break;
        case concord::CRS::ENU:
            os << "ENU";
            break;
        }
        os << "\nDATUM: " << fc.datum.lat << ", " << fc.datum.lon << ", " << fc.datum.alt << "\n"
           << "HEADING: " << fc.heading.yaw << "\n";
        os << "FEATURES: " << fc.features.size() << "\n";

        for (auto const &f : fc.features) {
            auto &v = f.geometry;
            if (std::get_if<concord::Polygon>(&v)) {
                std::cout << "  POLYGON\n";
            } else if (std::get_if<concord::Line>(&v)) {
                std::cout << "  LINE\n";
            } else if (std::get_if<concord::Path>(&v)) {
                std::cout << "  PATH\n";
            } else if (std::get_if<concord::Point>(&v)) {
                std::cout << "   POINT\n";
            }
            if (f.properties.size() > 0)
                std::cout << "    PROPS:" << f.properties.size() << "\n";
        }

        return os;
    }

} // namespace geoson
