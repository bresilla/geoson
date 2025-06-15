#pragma once

#include "parser.hpp"
#include "types.hpp"
#include "writter.hpp"

// Convenient aliases for common operations
namespace geoson {

    // Read function alias
    inline FeatureCollection read(const std::filesystem::path &file) { return ReadFeatureCollection(file); }

    // Write function aliases - with CRS choice
    inline void write(const FeatureCollection &fc, const std::filesystem::path &outPath, CRS outputCrs) {
        WriteFeatureCollection(fc, outPath, outputCrs);
    }

    // Write function alias - uses original input CRS format for output
    // Note: Internal representation is always Point coordinates, but output format
    // matches the original input file's CRS (stored in fc.crs for reference)
    inline void write(const FeatureCollection &fc, const std::filesystem::path &outPath) {
        WriteFeatureCollection(fc, outPath);
    }

} // namespace geoson
