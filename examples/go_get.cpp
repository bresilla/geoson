#include "geoget/geoget.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    geoget::PolygonDrawer app;
    // if (!app.start(8080)) {
    //     std::cerr << "Failed to start server" << std::endl;
    //     return 1;
    // }
    //
    // std::cout << "1. Test single point selection" << std::endl;
    // std::cout << "Open http://localhost:8080 in your browser" << std::endl;
    // std::cout << "Click to select a point, then click Done" << std::endl;
    //
    // app.collect_single_point();
    // const auto& all_points = app.get_all_single_points();
    // std::cout << "\nCollected " << all_points.size() << " points:" << std::endl;
    // for (size_t i = 0; i < all_points.size(); ++i) {
    //     std::cout << "  Point " << (i + 1) << ": " << all_points[i].lat << ", " << all_points[i].lon << std::endl;
    // }
    //
    // // Wait a bit for socket to be fully released
    // std::cout << "\nWaiting for socket to be released..." << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(2));

    // Restart for polygon mode on different port
    if (!app.start(8081)) {
        std::cerr << "Failed to restart server" << std::endl;
        return 1;
    }

    std::cout << "\n2. Test polygon drawing" << std::endl;
    std::cout << "Open http://localhost:8081 in your browser" << std::endl;
    std::cout << "Click points to draw a polygon, then click Done" << std::endl;

    app.collect_points();
    const auto &all_polygons = app.get_all_polygons();

    std::cout << "\nCollected " << all_polygons.size() << " polygons:" << std::endl;
    for (size_t p = 0; p < all_polygons.size(); ++p) {
        std::cout << "  Polygon " << (p + 1) << " with " << all_polygons[p].size() << " points:" << std::endl;
        for (size_t i = 0; i < all_polygons[p].size(); ++i) {
            std::cout << "    [" << i << "]: " << all_polygons[p][i].lat << ", " << all_polygons[p][i].lon << std::endl;
        }
    }

    return 0;
}
