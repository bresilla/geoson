#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "concord/types_basic.hpp"
#include "concord/types_line.hpp"
#include "concord/types_path.hpp"
#include "concord/types_polygon.hpp"

namespace geoson {

    namespace op {

        inline nlohmann::json ReadFeatureCollection(const std::filesystem::path &file) {
            std::ifstream ifs(file);
            if (!ifs)
                throw std::runtime_error("geoson::ReadFeatureCollection(): cannot open \"" + file.string() + '\"');

            nlohmann::json j;
            ifs >> j; // parse file into JSON

            // Ensure it’s an object with a string "type"
            if (!j.is_object() || !j.contains("type") || !j["type"].is_string())
                throw std::runtime_error(
                    "geoson::ReadFeatureCollection(): top-level object has no string 'type' field");

            auto type = j["type"].get<std::string>();

            // If it’s already a FeatureCollection, return as-is
            if (type == "FeatureCollection")
                return j;

            // If it’s a single Feature, wrap it into a FeatureCollection
            if (type == "Feature")
                return nlohmann::json{{"type", "FeatureCollection"}, {"features", nlohmann::json::array({j})}};

            // Otherwise it must be a bare geometry – wrap that into a one-element FeatureCollection
            nlohmann::json feat = {{"type", "Feature"}, {"geometry", j}, {"properties", nlohmann::json::object()}};
            return nlohmann::json{{"type", "FeatureCollection"}, {"features", nlohmann::json::array({feat})}};
        }

    } // namespace op

    using json = nlohmann::json;

    /// Concord geometry variant
    using ConcordGeometry = std::variant<concord::Point, concord::Line, concord::Path, concord::Polygon>;

    /// A single GeoJSON Feature (geometry + string properties + optional id)
    struct Feature {
        ConcordGeometry geometry;
        std::unordered_map<std::string, std::string> properties;
        std::optional<std::string> id;
    };

    /// The full parsed FeatureCollection, including optional "crs"
    struct FeatureCollection {
        std::optional<std::string> crs;
        std::vector<Feature> features;
    };

    /** Parse a JSON object of key→value into a map<string,string>.
     *  Strings are unwrapped, others are dumped to JSON.
     */
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

    /** [lon,lat,(alt)] → Concord::Point */
    inline concord::Point parsePoint(const json &coords, const concord::Datum &datum = {}) {
        double lon = coords.at(0).get<double>();
        double lat = coords.at(1).get<double>();
        double alt = coords.size() > 2 ? coords.at(2).get<double>() : 0.0;
        return concord::Point{concord::WGS{lat, lon, alt}, datum};
    }

    /** LineString → Line (2 pts) or Path (>2 pts) */
    inline ConcordGeometry parseLineString(const json &coords, const concord::Datum &datum = {}) {
        std::vector<concord::Point> pts;
        pts.reserve(coords.size());
        for (auto const &c : coords)
            pts.push_back(parsePoint(c, datum));
        if (pts.size() == 2)
            return concord::Line{pts[0], pts[1]};
        else
            return concord::Path{pts};
    }

    /** Polygon → Concord::Polygon (exterior ring only) */
    inline concord::Polygon parsePolygon(const json &coords, const concord::Datum &datum = {}) {
        std::vector<concord::Point> pts;
        auto const &ring = coords.at(0);
        pts.reserve(ring.size());
        for (auto const &c : ring)
            pts.push_back(parsePoint(c, datum));
        return concord::Polygon{pts};
    }

    /** Recursively parse any GeoJSON geometry into a flat list of ConcordGeometry */
    inline std::vector<ConcordGeometry> parseGeometry(const json &geom, const concord::Datum &datum = {}) {
        std::vector<ConcordGeometry> out;
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

    /**
     * Read a GeoJSON file (bare geometry, Feature, or FeatureCollection),
     * capture optional top-level "crs", and parse all Features into Concord.
     */
    inline FeatureCollection ReadFeatureCollection(const std::filesystem::path &file,
                                                   const concord::Datum &datum = {}) {
        // 1) Load + normalize to FeatureCollection JSON
        auto fc_json = op::ReadFeatureCollection(file);

        // 2) Grab optional "crs" field if it’s a string
        std::optional<std::string> crs;
        if (fc_json.contains("crs") && fc_json["crs"].is_string()) {
            crs = fc_json["crs"].get<std::string>();
        }

        // 3) Parse each Feature
        FeatureCollection fc;
        fc.crs = crs;
        fc.features.reserve(fc_json.at("features").size());

        for (auto const &feat : fc_json.at("features")) {
            if (feat.value("geometry", json{}).is_null())
                continue;

            // parse one or more Concord geometries
            auto geoms = parseGeometry(feat.at("geometry"), datum);

            // parse properties into map<string,string>
            auto props = parseProperties(feat.value("properties", json::object()));

            // optional id → always as string
            std::optional<std::string> id;
            if (feat.contains("id")) {
                id = feat.at("id").dump();
            }

            // one entry per geometry
            for (auto &g : geoms) {
                fc.features.push_back(Feature{std::move(g), props, id});
            }
        }

        return fc;
    }

} // namespace geoson
