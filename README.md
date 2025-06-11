
<img align="right" width="26%" src="./misc/logo.png">

Geoson
===

A modern C++20 library for reading, writing, and manipulating GeoJSON files with type-safe geometry handling.

## Overview

Geoson is a high-performance C++ library that provides a clean interface for working with GeoJSON files ([RFC 7946](https://tools.ietf.org/html/rfc7946)). Built on top of [JSON for Modern C++](https://github.com/nlohmann/json), it converts GeoJSON elements into strongly-typed [Concord](https://github.com/smolfetch/concord) geometries for precise spatial operations.

## Features

### 🗂️ **Comprehensive GeoJSON Support**
- **FeatureCollection**: Read/write complete GeoJSON feature collections
- **Features**: Individual geographic features with properties
- **Geometries**: Point, Line, Path, Polygon support
- **Multi-geometries**: MultiPoint, MultiLineString, MultiPolygon
- **GeometryCollection**: Nested geometry collections

### 🌍 **Coordinate Reference Systems**
- **WGS84** (EPSG:4326) - World Geodetic System
- **ENU** (East-North-Up) - Local coordinate systems
- **ECEF** - Earth-Centered, Earth-Fixed coordinates
- Automatic CRS parsing and validation

### 📍 **Spatial Data Handling**
- **Datum Support**: Reference point coordinates (lat, lon, alt)
- **Heading/Orientation**: Euler angle support for directional data
- **Type-safe Geometries**: Compile-time geometry type checking
- **Property Management**: String-based metadata handling

### 🔧 **Developer Experience**
- **Pretty Printing**: Human-readable output for debugging
- **Error Handling**: Comprehensive error reporting with context
- **File I/O**: Direct file reading/writing with path validation
- **Robust Parsing**: Handles malformed data gracefully

## Dependencies

- [Concord](https://github.com/smolfetch/concord) - Geometry and coordinate system handling
- [JSON for Modern C++](https://github.com/nlohmann/json) - JSON parsing and serialization

## Quick Start

### Basic Usage

```cpp
#include "geoson/geoson.hpp"
#include <iostream>

int main() {
    try {
        // Read GeoJSON file
        auto fc = geoson::ReadFeatureCollection("data.geojson");
        
        // Print summary information
        std::cout << fc << std::endl;
        
        // Modify spatial reference
        fc.datum.lat += 0.1;  // Adjust latitude
        fc.heading.yaw = 45.0; // Set heading
        
        // Save modified data
        geoson::WriteFeatureCollection(fc, "modified.geojson");
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

### Creating Geometries

```cpp
#include "geoson/geoson.hpp"

// Set up coordinate system
concord::CRS crs = concord::CRS::WGS;
concord::Datum datum{52.0, 5.0, 0.0};  // Amsterdam coordinates
concord::Euler heading{0.0, 0.0, 0.0};

// Create different geometry types
std::vector<geoson::Feature> features;

// Point feature
concord::Point point{concord::WGS{52.1, 5.1, 10.0}, datum};
std::unordered_map<std::string, std::string> pointProps;
pointProps["name"] = "Amsterdam Central";
pointProps["type"] = "landmark";
features.emplace_back(geoson::Feature{point, pointProps});

// Line feature (2 points)
concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};
concord::Point end{concord::WGS{52.2, 5.2, 0.0}, datum};
concord::Line line{start, end};
std::unordered_map<std::string, std::string> lineProps;
lineProps["name"] = "Main Street";
features.emplace_back(geoson::Feature{line, lineProps});

// Path feature (multiple points)
std::vector<concord::Point> pathPoints = {
    concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},
    concord::Point{concord::WGS{52.15, 5.15, 0.0}, datum},
    concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum}
};
concord::Path path{pathPoints};
features.emplace_back(geoson::Feature{path, {}});

// Polygon feature
std::vector<concord::Point> polygonPoints = {
    concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},
    concord::Point{concord::WGS{52.2, 5.1, 0.0}, datum},
    concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum},
    concord::Point{concord::WGS{52.1, 5.2, 0.0}, datum},
    concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum}  // Closed ring
};
concord::Polygon polygon{polygonPoints};
features.emplace_back(geoson::Feature{polygon, {}});

// Create feature collection
geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

// Write to file
geoson::WriteFeatureCollection(fc, "my_features.geojson");
```

### Working with Properties

```cpp
// Access feature properties
for (const auto& feature : fc.features) {
    // Check geometry type
    if (std::holds_alternative<concord::Point>(feature.geometry)) {
        auto point = std::get<concord::Point>(feature.geometry);
        std::cout << "Point at: " << point.wgs.lat << ", " << point.wgs.lon << std::endl;
    }
    
    // Access properties
    if (feature.properties.contains("name")) {
        std::cout << "Feature name: " << feature.properties.at("name") << std::endl;
    }
}
```

## Supported GeoJSON Features

| Feature | Status | Notes |
|---------|--------|-------|
| Point | ✅ | Single coordinate |
| LineString | ✅ | 2 points → Line, 3+ points → Path |
| Polygon | ✅ | Exterior ring only |
| MultiPoint | ✅ | Multiple Point geometries |
| MultiLineString | ✅ | Multiple LineString geometries |
| MultiPolygon | ✅ | Multiple Polygon geometries |
| GeometryCollection | ✅ | Nested geometry collections |
| Feature | ✅ | Geometry + properties |
| FeatureCollection | ✅ | Collection of features |
| Custom CRS | ✅ | WGS84, ENU, ECEF support |

## Error Handling

Geoson provides comprehensive error handling with descriptive messages:

```cpp
try {
    auto fc = geoson::ReadFeatureCollection("invalid.geojson");
} catch (const std::runtime_error& e) {
    // Catches parsing errors, file I/O errors, validation errors
    std::cerr << "Parsing failed: " << e.what() << std::endl;
}

// Example error messages:
// - "Cannot open for write: /invalid/path/file.geojson"
// - "Unknown CRS string: EPSG:9999"
// - "'properties' missing array 'datum' of ≥3 numbers"
// - "geoson::ReadFeatureCollection(): cannot open \"missing.geojson\""
```

## Building

```bash
# Configure with tests enabled
cmake -B build -DGEOSON_ENABLE_TESTS=ON

# Build
make -C build

# Run tests
make -C build test
```

## Acknowledgements

This library was originally inspired by [libgeojson](https://github.com/psalvaggio/libgeojson), but has been completely rewritten with modern C++20 features and a focus on type safety and performance.
