
<img align="right" width="26%" src="./misc/logo.png">

Geoson
===

A C++ library for working with geojson files.


This library serves as wrapper around [JSON for Modern C++](https://github.com/nlohmann/json) for the GeoJSON ([RFC 7946](https://tools.ietf.org/html/rfc7946)) standard.

At the code, this will translate every GEoJSON element into a [Concord](https://github.com/smolfetch/concord) geometry. This way you can have better control over the geometry.

### Dependencies

This library depends on [Concord](https://github.com/smolfetch/concord) and [JSON for Modern C++](https://github.com/nlohmann/json).

### Usage

```cpp
#include <geoson/parser.hpp>

int main() {
    auto fc = geoson::ReadFeatureCollection("path/to/file.geojson");
}
```

### Acknowledgements

This library was originally forked from [libgeojson](https://github.com/psalvaggio/libgeojson), but has been completely rewritten. However, it borrows ideas from the original library.
