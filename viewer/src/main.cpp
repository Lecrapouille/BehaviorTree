//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2025 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//*****************************************************************************

#include "Application.hpp"

#include <iostream>

// ----------------------------------------------------------------------------
//! \brief Parse command line arguments.
//! \param[in] argc The number of command line arguments.
//! \param[in] argv The command line arguments.
//! \param[out] port The port number.
//! \return true if the arguments are valid, false otherwise.
// ----------------------------------------------------------------------------
static bool parse_arguments(int argc, char* argv[], uint16_t& port)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            std::cout << "Usage: " << argv[0] << " [-p|--port PORT]"
                      << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -h, --help     Show this help message" << std::endl;
            std::cout
                << "  -p, --port     Specify the port number (default: 9090)"
                << std::endl;
            return EXIT_SUCCESS;
        }
        else if (arg == "-p" || arg == "--port")
        {
            if (i + 1 < argc)
            {
                try
                {
                    port = uint16_t(std::stoul(argv[++i]));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Invalid port number: " << argv[i]
                              << std::endl;
                    return false;
                }
            }
            else
            {
                std::cerr << "Error: Port number not specified" << std::endl;
                return false;
            }
        }
    }

    return true;
}

int main(int argc, char* argv[])
{
    bt::BehaviorTreeVisualizer application;

    // Default port
    uint16_t port = 9090;

    // Parse command line arguments
    if (!parse_arguments(argc, argv, port))
    {
        return EXIT_FAILURE;
    }

    // Initialize the application
    unsigned int antialiasing_level = 0;
    if (!application.initialize(1280, 720, port, antialiasing_level))
    {
        std::cerr << "Failed to initialize the application" << std::endl;
        return EXIT_FAILURE;
    }

    // Run the application
    application.run();
    return EXIT_SUCCESS;
}