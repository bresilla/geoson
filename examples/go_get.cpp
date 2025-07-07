#include "geoget/geoget.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    geoget::PolygonDrawer app;

    // First, add datum
    if (!app.start(8080)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "1. Select datum point" << std::endl;
    std::cout << "Open http://localhost:8080 in your browser" << std::endl;
    std::cout << "Click to select the datum point (reference point), then click Done" << std::endl;

    auto datum = app.add_datum();
    std::cout << "Datum added: " << datum.lat << ", " << datum.lon << std::endl;

    // Wait a bit for socket to be fully released
    std::cout << "\nWaiting for socket to be released..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Restart for polygon mode on different port
    if (!app.start(8082)) {
        std::cerr << "Failed to restart server" << std::endl;
        return 1;
    }

    std::cout << "\n2. Test polygon drawing" << std::endl;
    std::cout << "Open http://localhost:8081 in your browser" << std::endl;
    std::cout << "Click points to draw a polygon, then click Done" << std::endl;

    try {
        const auto polygons = app.get_polygons();

        std::cout << "\nCollected " << polygons.size() << " polygons (converted to concord::Polygon):" << std::endl;
        for (size_t p = 0; p < polygons.size(); ++p) {
            std::cout << "  Polygon " << (p + 1) << " with " << polygons[p].numVertices() << " vertices:" << std::endl;
            for (const auto &vertex : polygons[p].getPoints()) {
                std::cout << "    ENU: (" << vertex.x << ", " << vertex.y << ", " << vertex.z << ")" << std::endl;
            }
            std::cout << "    Area: " << polygons[p].area() << " m²" << std::endl;
            std::cout << "    Perimeter: " << polygons[p].perimeter() << " m" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
