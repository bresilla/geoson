
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

### 🌍 **Coordinate Reference Systems & Multi-Flavor Support**
- **WGS84** (EPSG:4326) - Global geodetic coordinates (latitude, longitude, altitude)
- **ENU** (East-North-Up) - Local coordinate systems relative to a datum point
- **Automatic CRS Detection**: Parses CRS from GeoJSON properties
- **Dual-Flavor I/O**: Read and write in both WGS84 and ENU coordinate systems
- **Intelligent Coordinate Conversion**: Seamless transformation between coordinate systems

### 📍 **Datum-Centric Spatial Handling**
- **Reference Point Coordinates**: Critical datum (lat, lon, alt) for ENU transformations
- **Coordinate System Anchoring**: All ENU coordinates are relative to the datum
- **Heading/Orientation**: Euler angles for directional data and coordinate frame alignment
- **Type-safe Geometries**: Compile-time geometry type checking with CRS awareness
- **Bi-directional Conversion**: WGS ↔ ENU transformations preserve spatial relationships

### 🔧 **Developer Experience**
- **Pretty Printing**: Human-readable output for debugging
- **Error Handling**: Comprehensive error reporting with context
- **File I/O**: Direct file reading/writing with path validation
- **Robust Parsing**: Handles malformed data gracefully

## Dependencies

- [Concord](https://github.com/smolfetch/concord) - Geometry and coordinate system handling
- [JSON for Modern C++](https://github.com/nlohmann/json) - JSON parsing and serialization

## CRS-Aware Parsing and Writing

Geoson supports **two distinct GeoJSON flavors** based on coordinate reference systems, with the **datum playing a critical role** in coordinate transformations:

### WGS84 Flavor (EPSG:4326)
- **Global coordinates**: Latitude, longitude, altitude in decimal degrees
- **Standard GeoJSON**: Follows RFC 7946 specification exactly  
- **Datum as origin**: The datum serves as the reference point for any local transformations
- **Direct usage**: Coordinates are used as-is for global positioning

### ENU Flavor (East-North-Up)
- **Local coordinates**: Relative East, North, Up distances in meters
- **Datum-centered**: All coordinates are relative to the datum point (lat, lon, alt)
- **High precision**: Ideal for surveying, robotics, and local area applications
- **Meter-based**: Direct distance measurements without geodetic calculations

### The Critical Role of Datum

The **datum** is not just metadata—it's the **spatial anchor** that defines the coordinate system:

```cpp
// Datum coordinates (lat=52.0°N, lon=5.0°E, alt=0m) - Amsterdam area
concord::Datum datum{52.0, 5.0, 0.0};

// WGS84 Flavor: Global coordinates
// Point at latitude 52.1°, longitude 5.1° (about 11km northeast of datum) 
concord::Point wgs_point{concord::WGS{52.1, 5.1, 10.0}, datum/* converts to ENU internally */};

// ENU Flavor: Local coordinates  
// Point at 1000m East, 500m North, 10m Up from the datum
concord::Point enu_point{1000.0, 500.0, 10.0};
```

### Parsing Behavior

When **reading** GeoJSON files, the coordinate interpretation depends on the `crs` property:

```cpp
// Reading WGS84 GeoJSON (crs: "EPSG:4326")
// Coordinates [5.1, 52.1, 10.0] are interpreted as:
// → longitude=5.1°, latitude=52.1°, altitude=10.0m
// → Converted to ENU relative to datum during parsing
auto wgs_fc = geoson::ReadFeatureCollection("global_data.geojson");

// Reading ENU GeoJSON (crs: "ENU") 
// Coordinates [1000.0, 500.0, 10.0] are interpreted as:
// → east=1000m, north=500m, up=10.0m (relative to datum)
// → Used directly as local coordinates
auto enu_fc = geoson::ReadFeatureCollection("local_survey.geojson");
```

### Writing Behavior

When **writing** GeoJSON files, you specify the desired output flavor:

```cpp
// Same internal geometry data...
geoson::FeatureCollection fc{geoson::CRS::WGS, datum, heading, features};

// Write as WGS84 GeoJSON (global coordinates)
// ENU coordinates are converted back to lat/lon/alt
geoson::WriteFeatureCollection(fc, "output_global.geojson");
// Output: [5.1, 52.1, 10.0] (longitude, latitude, altitude)

// Write as ENU GeoJSON (local coordinates) 
fc.crs = geoson::CRS::ENU;
geoson::WriteFeatureCollection(fc, "output_local.geojson");
// Output: [1000.0, 500.0, 10.0] (east, north, up relative to datum)
```

### Datum Precision Impact

The datum location directly affects coordinate precision and usability:

```cpp
// High-precision datum for local surveying
concord::Datum survey_datum{52.123456, 5.654321, 12.345};

// All ENU coordinates will be relative to this precise reference point
// Perfect for: construction sites, precision agriculture, robotics

// Coarse datum for regional work  
concord::Datum regional_datum{52.0, 5.0, 0.0};

// Suitable for: city-level mapping, general GIS applications
```

### Multi-Flavor Workflow Example

```cpp
// 1. Read survey data (ENU flavor - high precision local coordinates)
auto survey_data = geoson::ReadFeatureCollection("survey_enu.geojson");

// 2. Process with local algorithms (distances, areas, etc.)
// ... spatial processing on ENU coordinates ...

// 3. Export for global GIS (WGS84 flavor - standard GeoJSON)
survey_data.crs = geoson::CRS::WGS;  // Switch output flavor
geoson::WriteFeatureCollection(survey_data, "for_gis_global.geojson");

// 4. The datum ensures perfect coordinate transformation between flavors
```

### Best Practices

1. **Choose datum wisely**: Place it at the center of your area of interest
2. **Consistent datum**: Use the same datum across related datasets  
3. **Precision matters**: More decimal places in datum = higher local accuracy
4. **Flavor selection**: Use ENU for local work, WGS84 for sharing/interoperability

## Quick Start

### Basic Usage

```cpp
#include "geoson/geoson.hpp"
#include <iostream>

int main() {
    try {
        // Read GeoJSON file (automatically detects WGS84 or ENU flavor)
        auto fc = geoson::ReadFeatureCollection("data.geojson");
        
        // Print summary information
        std::cout << fc << std::endl;
        
        // Modify spatial reference (affects all coordinate transformations)
        fc.datum.lat += 0.1;  // Shift datum north by 0.1 degrees
        fc.heading.yaw = 45.0; // Set heading to 45 degrees
        
        // Save in original flavor
        geoson::WriteFeatureCollection(fc, "modified.geojson");
        
        // Convert to different flavor for export
        fc.crs = geoson::CRS::WGS;  // Force WGS84 output
        geoson::WriteFeatureCollection(fc, "modified_global.geojson");
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

### Creating Geometries with CRS Awareness

```cpp
#include "geoson/geoson.hpp"

// Set up coordinate system and datum (critical for ENU transformations)
geoson::CRS crs = geoson::CRS::WGS;  // Will output in WGS84 flavor
concord::Datum datum{52.0, 5.0, 0.0};  // Amsterdam area reference point
concord::Euler heading{0.0, 0.0, 0.0};

// Create different geometry types
std::vector<geoson::Feature> features;

// Method 1: WGS coordinates (global lat/lon/alt)
// These coordinates are automatically converted to ENU internally using the datum
concord::Point wgs_point{concord::WGS{52.1, 5.1, 10.0}, datum};
std::unordered_map<std::string, std::string> pointProps;
pointProps["name"] = "Amsterdam Central";
pointProps["coordinate_type"] = "WGS84";
features.emplace_back(geoson::Feature{wgs_point, pointProps});

// Method 2: ENU coordinates (local east/north/up in meters)
// These are relative to the datum point
concord::Point enu_point{1000.0, 500.0, 10.0};  // 1km east, 500m north, 10m up from datum
std::unordered_map<std::string, std::string> enuProps;
enuProps["name"] = "Local Survey Point";
enuProps["coordinate_type"] = "ENU";
features.emplace_back(geoson::Feature{enu_point, enuProps});

// Line feature with mixed coordinate input
concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};  // Global coordinates
concord::Point end{500.0, 1000.0, 0.0};  // Local ENU coordinates
concord::Line line{start, end};
std::unordered_map<std::string, std::string> lineProps;
lineProps["name"] = "Mixed Coordinate Line";
features.emplace_back(geoson::Feature{line, lineProps});

// Path feature (multiple points) - demonstrating coordinate flexibility
std::vector<concord::Point> pathPoints = {
    concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},    // Global coordinates
    concord::Point{1500.0, 1500.0, 0.0},                    // ENU coordinates  
    concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum}     // Back to global
};
concord::Path path{pathPoints};
std::unordered_map<std::string, std::string> pathProps;
pathProps["name"] = "Survey Route";
pathProps["mixed_coordinates"] = "true";
features.emplace_back(geoson::Feature{path, pathProps});

// Polygon feature with consistent ENU coordinates for high precision
std::vector<concord::Point> polygonPoints = {
    concord::Point{0.0, 0.0, 0.0},        // At datum point
    concord::Point{100.0, 0.0, 0.0},      // 100m east
    concord::Point{100.0, 100.0, 0.0},    // 100m east, 100m north  
    concord::Point{0.0, 100.0, 0.0},      // 100m north
    concord::Point{0.0, 0.0, 0.0}         // Closed ring
};
concord::Polygon polygon{polygonPoints};
std::unordered_map<std::string, std::string> polyProps;
polyProps["name"] = "Survey Plot";
polyProps["area_m2"] = "10000";  // 100m x 100m = 10,000 m²
features.emplace_back(geoson::Feature{polygon, polyProps});

// Create feature collection
geoson::FeatureCollection fc{crs, datum, heading, std::move(features)};

// Write in WGS84 flavor (global coordinates)
geoson::WriteFeatureCollection(fc, "global_output.geojson");
// Coordinates like [0.0, 0.0, 0.0] become [5.0, 52.0, 0.0] (datum location)

// Write in ENU flavor (local coordinates)
fc.crs = geoson::CRS::ENU;
geoson::WriteFeatureCollection(fc, "local_output.geojson");  
// Coordinates remain as [0.0, 0.0, 0.0] (relative to datum)
```

### Coordinate System Conversion Examples

```cpp
// Reading and converting between flavors
auto global_data = geoson::ReadFeatureCollection("global.geojson");  // WGS84 input
// Internal processing uses ENU coordinates relative to datum

// Export as local survey data
global_data.crs = geoson::CRS::ENU;
geoson::WriteFeatureCollection(global_data, "local_survey.geojson"); // ENU output

// The datum acts as the conversion anchor:
// - WGS input [5.1, 52.1, 10.0] → ENU internal [~11132m, ~11119m, 10.0m] → ENU output [11132, 11119, 10]  
// - ENU input [1000, 500, 10] → ENU internal [1000, 500, 10] → WGS output [~5.014, ~52.005, 10]
```

### Working with Properties and CRS Detection

```cpp
// Access feature properties and understand coordinate systems
for (const auto& feature : fc.features) {
    // Check geometry type
    if (std::holds_alternative<concord::Point>(feature.geometry)) {
        auto point = std::get<concord::Point>(feature.geometry);
        
        // Access the underlying ENU coordinates (always available)
        std::cout << "ENU coordinates: " << point.enu.east << ", " 
                  << point.enu.north << ", " << point.enu.up << std::endl;
                  
        // Convert back to WGS if needed
        auto wgs_coords = point.enu.toWGS(fc.datum);
        std::cout << "WGS coordinates: " << wgs_coords.lat << ", " 
                  << wgs_coords.lon << ", " << wgs_coords.alt << std::endl;
    }
    
    // Access properties and CRS metadata
    if (feature.properties.contains("name")) {
        std::cout << "Feature name: " << feature.properties.at("name") << std::endl;
    }
    
    if (feature.properties.contains("coordinate_type")) {
        std::cout << "Original coordinate type: " << feature.properties.at("coordinate_type") << std::endl;
    }
}

// Check the FeatureCollection's CRS and datum
std::cout << "Collection CRS: " << (fc.crs == geoson::CRS::WGS ? "WGS84" : "ENU") << std::endl;
std::cout << "Datum: [" << fc.datum.lat << ", " << fc.datum.lon << ", " << fc.datum.alt << "]" << std::endl;
```

## Supported GeoJSON Features

| Feature | Status | CRS Support | Notes |
|---------|--------|-------------|-------|
| Point | ✅ | WGS84 + ENU | Single coordinate with automatic conversion |
| LineString | ✅ | WGS84 + ENU | 2 points → Line, 3+ points → Path |
| Polygon | ✅ | WGS84 + ENU | Exterior ring only, datum-aware coordinates |
| MultiPoint | ✅ | WGS84 + ENU | Multiple Point geometries |
| MultiLineString | ✅ | WGS84 + ENU | Multiple LineString geometries |
| MultiPolygon | ✅ | WGS84 + ENU | Multiple Polygon geometries |
| GeometryCollection | ✅ | WGS84 + ENU | Nested geometry collections |
| Feature | ✅ | WGS84 + ENU | Geometry + properties |
| FeatureCollection | ✅ | WGS84 + ENU | Collection with CRS and datum metadata |
| Custom CRS | ✅ | WGS84/ENU | Automatic detection and conversion |

### CRS Property Support

```json
{
  "type": "FeatureCollection",
  "properties": {
    "crs": "EPSG:4326",           // or "ENU" for local coordinates
    "datum": [52.0, 5.0, 0.0],   // Critical: lat, lon, alt reference point
    "heading": 0.0                // Optional: orientation in degrees
  },
  "features": [...]
}
```

## Error Handling

Geoson provides comprehensive error handling with descriptive messages, including CRS and datum validation:

```cpp
try {
    auto fc = geoson::ReadFeatureCollection("invalid.geojson");
} catch (const std::runtime_error& e) {
    // Catches parsing errors, file I/O errors, validation errors, CRS errors
    std::cerr << "Parsing failed: " << e.what() << std::endl;
}

// Example error messages for CRS and datum issues:
// - "Cannot open for write: /invalid/path/file.geojson"
// - "Unknown CRS string: EPSG:9999"
// - "'properties' missing string 'crs'"
// - "'properties' missing array 'datum' of ≥3 numbers"
// - "'properties' missing numeric 'heading'"
// - "geoson::ReadFeatureCollection(): cannot open \"missing.geojson\""

// CRS-specific validation errors:
try {
    geoson::parseCRS("invalid_crs");
} catch (const std::runtime_error& e) {
    // "Unknown CRS string: invalid_crs"
}

// Coordinate parsing errors (datum-dependent):
try {
    // Invalid coordinate array
    nlohmann::json coords = {5.1}; // Missing latitude
    geoson::parsePoint(coords, datum, geoson::CRS::WGS);
} catch (const nlohmann::json::out_of_range& e) {
    // Coordinate array access error
}
```

## Building

```bash
# Configure with tests enabled
make compile

# Build all targets
make build

# Run comprehensive test suite (includes CRS conversion and datum precision tests)
make test
```

## Use Cases by CRS Flavor

### WGS84 Flavor - Global Applications
- **Web mapping**: Direct compatibility with web maps (Leaflet, Google Maps)
- **GIS integration**: Standard format for QGIS, ArcGIS, PostGIS
- **Data sharing**: Universal format for geographic data exchange
- **GPS applications**: Direct use of GPS coordinates without conversion

### ENU Flavor - Local High-Precision Applications
- **Construction & surveying**: Millimeter-precision local measurements
- **Robotics**: Path planning and navigation in local coordinate frames
- **Precision agriculture**: Field operations with centimeter accuracy
- **Engineering**: CAD integration and local coordinate system alignment

### Performance Considerations

- **Memory efficiency**: Internal ENU storage minimizes coordinate conversion overhead
- **Precision preservation**: Datum-based transformations maintain spatial accuracy
- **Conversion cost**: WGS↔ENU transformations are computationally lightweight
- **File size**: ENU coordinates often have fewer decimal places (smaller files)

## Acknowledgements

This library was originally inspired by [libgeojson](https://github.com/psalvaggio/libgeojson), but has been completely rewritten with modern C++20 features, CRS-aware coordinate handling, and a focus on type safety and datum-based spatial precision.
