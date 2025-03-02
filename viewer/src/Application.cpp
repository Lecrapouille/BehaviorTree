//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2024 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
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
#include "project_info.hpp"

#include <yaml-cpp/yaml.h>

#include <cstring>
#include <thread>
#include <iostream>

namespace bt {

// ----------------------------------------------------------------------------
BehaviorTreeViewer::BehaviorTreeViewer()
    : m_path(GET_DATA_PATH)
{}

// ----------------------------------------------------------------------------
BehaviorTreeViewer::~BehaviorTreeViewer()
{
    // Stop the debug server if it is active
    if (m_server != nullptr)
    {
        m_server->stop();
        m_server = nullptr;
    }

    // Close the SFML window
    m_render_context.window.close();
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::resetCamera()
{
    m_render_context.camera = m_render_context.window.getDefaultView();
    m_render_context.window.setView(m_render_context.camera);
    m_render_context.camera.setCenter(0, 0);
    m_render_context.camera.setSize(1280, 720);
}

// ----------------------------------------------------------------------------
bool BehaviorTreeViewer::initialize(uint32_t p_width, uint32_t p_height, uint16_t p_port)
{
    // Store the dimensions and port
    m_render_context.width = p_width;
    m_render_context.height = p_height;

    // Create the SFML window with anti-aliasing (4x MSAA)
    sf::ContextSettings settings;
    //settings.antialiasingLevel = 8; // High anti-aliasing level
    m_render_context.window.create(
        sf::VideoMode(m_render_context.width, m_render_context.height),
        "Behavior Tree Viewer", sf::Style::Default, settings);
    m_render_context.window.setFramerateLimit(60);

    // Load the resources
    if (!loadResources())
    {
        std::cerr << "Warning: Failed to load the resources" << std::endl;
    }

    // Initialize the background
    initializeBackground();

    // Initialize the camera with the default view
    resetCamera();

    // Initialize the help text
    initializeHelpText(p_port);

    // Initialize the tree renderer
    m_tree_renderer = std::make_unique<TreeRenderer>(m_render_context.window,
        m_render_context.font, m_render_context.icons);

    // Initialize the debug server with a callback to process messages
    m_server = std::make_unique<Server>(p_port, [this](const std::vector<uint8_t>& data)
    {
        if (m_tree_renderer != nullptr)
        {
            m_tree_renderer->handleMessage(data);
        }
    });

    // Start the server
    return m_server->start();
}

// ----------------------------------------------------------------------------
bool BehaviorTreeViewer::loadResources()
{
    namespace fs = std::filesystem;
    bool result = true;

    // Load the TTF font
    if (!m_render_context.font.loadFromFile(m_path.expand("fonts/MadimiOne-Regular.ttf")))
    {
        std::cerr << "Failed to load the TTF font" << std::endl;
        result = false;
    }

    // Load the icons
    fs::path icon_dir = fs::path(m_path.expand("icons/nodes"));
    if ((!fs::exists(icon_dir)) || (!fs::is_directory(icon_dir)))
    {
        std::cerr << "Icon directory not found: " << icon_dir << std::endl;
        result = false;
    }
    else
    {
        for (const auto& entry : fs::directory_iterator(icon_dir))
        {
            if ((entry.is_regular_file()) && (entry.path().extension() == ".png"))
            {
                // Extract the file name without extension to use as a key
                std::string icon_name = entry.path().stem().string();

                sf::Texture texture;
                if (texture.loadFromFile(entry.path().string()))
                {
                    m_render_context.icons[icon_name] = texture;
                }
                else
                {
                    std::cerr << "Failed to load the icon: " << entry.path() << std::endl;
                }

                std::cout << "Icon loaded: " << icon_name << std::endl;
            }
        }
    }

    return result;
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::initializeBackground()
{
    // Create a rectangle for the background with gradient
    m_render_context.background.setPrimitiveType(sf::Quads);
    m_render_context.background.resize(4);

    // Gradient colors inspired by the reference image
    sf::Color topColor(30, 40, 60);       // Dark blue at the top
    sf::Color bottomColor(15, 20, 35);    // Very dark blue at the bottom

    // Define the vertices of the rectangle with gradient
    float width = static_cast<float>(m_render_context.width);
    float height = static_cast<float>(m_render_context.height);
    m_render_context.background[0].position = sf::Vector2f(0.0f, 0.0f);
    m_render_context.background[1].position = sf::Vector2f(width, 0.0f);
    m_render_context.background[2].position = sf::Vector2f(width, height);
    m_render_context.background[3].position = sf::Vector2f(0.0f, height);

    // Apply the gradient colors
    m_render_context.background[0].color = topColor;
    m_render_context.background[1].color = topColor;
    m_render_context.background[2].color = bottomColor;
    m_render_context.background[3].color = bottomColor;
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::initializeHelpText(uint16_t p_port)
{
    m_render_context.help_text.setFont(m_render_context.font);
    m_render_context.help_text.setString("Waiting for the behavior tree on port " +
        std::to_string(p_port) + "...\n");
    m_render_context.help_text.setCharacterSize(20);
    m_render_context.help_text.setFillColor(sf::Color::White);
    m_render_context.help_text.setPosition(10, 10);
}

// ----------------------------------------------------------------------------
bool BehaviorTreeViewer::handleConnection(bool& p_was_connected)
{
    // Check if the connection state has changed
    bool is_connected = (m_server != nullptr) && m_server->isConnected();
    if (p_was_connected && !is_connected)
    {
        // The client has disconnected, reset the tree data
        std::cout << "Client disconnected, resetting tree data" << std::endl;
        if (m_tree_renderer != nullptr)
        {
            m_tree_renderer->reset();
        }
        resetCamera();
    }
    p_was_connected = is_connected;
    return is_connected;
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::run()
{
    // Variable to track the previous client-server connection state
    bool was_connected = false;

    while (m_render_context.window.isOpen())
    {
        sf::Event event;
        while (m_render_context.window.pollEvent(event))
        {
            // Close the application
            if (event.type == sf::Event::Closed)
            {
                m_render_context.window.close();
                return;
            }
        }

        // Handle the client-server connection
        bool is_connected = handleConnection(was_connected);

        // Draw the background with gradient
        m_render_context.window.clear();
        m_render_context.window.setView(m_render_context.window.getDefaultView());
        m_render_context.window.draw(m_render_context.background);

        // Draw the tree with the camera view
        if ((is_connected) && (m_tree_renderer != nullptr))
        {
            m_render_context.window.setView(m_render_context.camera);
            m_render_context.window.draw(*m_tree_renderer);
        }

        // Draw the help text if necessary
        if (!is_connected)
        {
            m_render_context.window.setView(m_render_context.window.getDefaultView());
            m_render_context.window.draw(m_render_context.help_text);
        }

        m_render_context.window.display();
    }
}

} // namespace bt