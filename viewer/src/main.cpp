#include "BehaviorTreeViewer.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    bt::BehaviorTreeViewer viewer;

    // Default port
    uint16_t port = 9090;

    // Get port from command line
    if (argc > 1)
    {
        port = uint16_t(std::stoul(argv[1]));
    }

    // Initialize the viewer
    if (!viewer.initialize(1280, 720, port))
    {
        std::cerr << "Failed to initialize the viewer" << std::endl;
        return EXIT_FAILURE;
    }

    // Run the viewer
    viewer.run();

    std::cout << "Fin du programme" << std::endl;
    return EXIT_SUCCESS;
}