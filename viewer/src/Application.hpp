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

#pragma once

#include "BehaviorTree/BehaviorTreeVisualizer.hpp"

#include "Path.hpp"
#include "Server.hpp"
#include "TreeRenderer.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace YAML {
class Node;
}

class NodeRenderer;
class DebugServer;

namespace bt {

// ****************************************************************************
//! \brief Application allowing to display in real time an application using a
//! behavior tree.
// ****************************************************************************
class BehaviorTreeViewer
{
public:

    BehaviorTreeViewer();
    ~BehaviorTreeViewer();

    // ------------------------------------------------------------------------
    //! \brief Initialize the viewer window.
    //! \param[in] p_width Width of the window.
    //! \param[in] p_height Height of the window.
    //! \param[in] p_port Port of the debug server.
    //! \return true if the initialization is successful.
    // ------------------------------------------------------------------------
    bool initialize(uint32_t p_width, uint32_t p_height, uint16_t p_port);

    // ------------------------------------------------------------------------
    //! \brief Run the main loop
    // ------------------------------------------------------------------------
    void run();

private: // Initialize the application

    // ------------------------------------------------------------------------
    //! \brief Load the resources (TTF font and PNG icons).
    //! \return true if the loading succeeded, false otherwise.
    // ------------------------------------------------------------------------
    bool loadResources();

    // ------------------------------------------------------------------------
    //! \brief Initialize the background of the application.
    // ------------------------------------------------------------------------
    void initializeBackground();

    // ------------------------------------------------------------------------
    //! \brief Initialize the help text.
    //! \param[in] p_port Port of the debug server.
    // ------------------------------------------------------------------------
    void initializeHelpText(uint16_t p_port);

    // ------------------------------------------------------------------------
    //! \brief Reset the camera.
    // ------------------------------------------------------------------------
    void resetCamera();

    // ------------------------------------------------------------------------
    //! \brief Handle the connection to the behavior tree.
    //! \param[in] p_was_connected True if the connection was previously
    //! established, false otherwise. \return True if the connection is
    //! successful, false otherwise.
    // ------------------------------------------------------------------------
    bool handleConnection(bool& p_was_connected);

private:

    struct RenderContext
    {
        //! \brief Width of the window
        uint32_t width;
        //! \brief Height of the window
        uint32_t height;
        //! \brief Window for the behavior tree
        sf::RenderWindow window;
        //! \brief Camera for the behavior tree
        sf::View camera;
        //! \brief Font of the node.
        sf::Font font;
        //! \brief Collection of icons.
        std::unordered_map<std::string, sf::Texture> icons;
        //! \brief Vertex array for the background gradient
        sf::VertexArray background;
        //! \brief Help text
        sf::Text help_text;
    };

    //! \brief Helper instance to find files like Linux $PATH environment
    //! variable. Used for example for loading font files.
    Path m_path;
    //! \brief Server for communication with the behavior tree
    std::unique_ptr<Server> m_server;
    //! \brief Render context
    RenderContext m_render_context;
    //! \brief Renderer for the behavior tree
    std::unique_ptr<TreeRenderer> m_tree_renderer;
};

} // namespace bt