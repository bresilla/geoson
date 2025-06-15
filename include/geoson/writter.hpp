#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "geoson/types.hpp"

namespace geoson {

    /// helper to turn a single Geometry into its GeoJSON object
    inline nlohmann::json geometryToJson(Geometry const &geom, const concord::Datum &datum, geoson::CRS crs) {
        // small helper to build coordinates based on CRS flavor
        auto ptCoords = [&](concord::Point const &p) {
            if (crs == geoson::CRS::ENU) {
                // ENU flavor: output coordinates directly as x,y,z
                return nlohmann::json::array({p.x, p.y, p.z});
            } else {
                // WGS flavor: convert Point to ENU with datum, then to WGS
                concord::ENU enu{p, datum};
                concord::WGS wgs = enu.toWGS();
                return nlohmann::json::array({wgs.lon, wgs.lat, wgs.alt});
            }
        };

        return std::visit(
            [&](auto const &shape) -> nlohmann::json {
                using T = std::decay_t<decltype(shape)>;
                nlohmann::json j;
                if constexpr (std::is_same_v<T, concord::Point>) {
                    j["type"] = "Point";
                    j["coordinates"] = ptCoords(shape);
                } else if constexpr (std::is_same_v<T, concord::Line>) {
                    j["type"] = "LineString";
                    j["coordinates"] = nlohmann::json::array({ptCoords(shape.getStart()), ptCoords(shape.getEnd())});
                } else if constexpr (std::is_same_v<T, concord::Path>) {
                    j["type"] = "LineString";
                    nlohmann::json arr = nlohmann::json::array();
                    for (auto const &p : shape.getPoints())
                        arr.push_back(ptCoords(p));
                    j["coordinates"] = std::move(arr);
                } else if constexpr (std::is_same_v<T, concord::Polygon>) {
                    j["type"] = "Polygon";
                    nlohmann::json rings = nlohmann::json::array();
                    nlohmann::json ring = nlohmann::json::array();
                    for (auto const &p : shape.getPoints())
                        ring.push_back(ptCoords(p));
                    rings.push_back(std::move(ring));
                    j["coordinates"] = std::move(rings);
                }
                return j;
            },
            geom);
    }

    /// turn one Feature into its GeoJSON object
    inline nlohmann::json featureToJson(Feature const &f, const concord::Datum &datum, geoson::CRS crs) {
        nlohmann::json j;
        j["type"] = "Feature";
        j["properties"] = nlohmann::json::object();
        for (auto const &kv : f.properties)
            j["properties"][kv.first] = kv.second;
        j["geometry"] = geometryToJson(f.geometry, datum, crs);
        return j;
    }

    /// serialize a full FeatureCollection to GeoJSON
    inline nlohmann::json toJson(FeatureCollection const &fc) {
        nlohmann::json j;
        j["type"] = "FeatureCollection";

        // top‐level properties
        {
            auto &P = j["properties"];
            P = nlohmann::json::object();

            // crs → string
            switch (fc.crs) {
            case geoson::CRS::WGS:
                P["crs"] = "EPSG:4326";
                break;
            case geoson::CRS::ENU:
                P["crs"] = "ENU";
                break;
            }

            // datum array
            P["datum"] = nlohmann::json::array({fc.datum.lat, fc.datum.lon, fc.datum.alt});

            // yaw only
            P["heading"] = fc.heading.yaw;
        }

        // features
        j["features"] = nlohmann::json::array();
        for (auto const &f : fc.features)
            j["features"].push_back(featureToJson(f, fc.datum, fc.crs));

        return j;
    }

    /// write GeoJSON out to disk (pretty‐printed)
    inline void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath) {
        auto j = toJson(fc);
        std::ofstream ofs(outPath);
        if (!ofs)
            throw std::runtime_error("Cannot open for write: " + outPath.string());
        ofs << j.dump(2) << "\n";
    }

} // namespace geoson
