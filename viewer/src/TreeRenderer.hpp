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

#include "ArcShape.hpp"
#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include "NodeShape.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace YAML {
class Node;
}

namespace bt {

// Forward declaration
class NodeShape;
class ArcShape;

// ****************************************************************************
//! \brief Class for rendering the behavior tree.
// ****************************************************************************
class TreeRenderer: public sf::Drawable
{
private:

    // ************************************************************************
    //! \brief Structure to store node information
    // ************************************************************************
    struct NodeInfo
    {
        uint32_t id;           //!< Node ID
        std::string name;      //!< Node name
        bt::Status status;     //!< Node status
        sf::Vector2f position; //!< Node position
        NodeInfo*
            parent; //!< Parent node (raw pointer because it's a back reference)
        std::vector<NodeInfo*> children;  //!< Children nodes (raw pointers
                                          //!< because they are back references)
        std::unique_ptr<NodeShape> shape; //!< Node shape
    };

public:

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] p_window Reference to the SFML window.
    //! \param[in] p_font Reference to the SFML font.
    //! \param[in] p_icons Reference to the SFML icons.
    // ------------------------------------------------------------------------
    explicit TreeRenderer(sf::RenderWindow& p_window,
                          sf::Font& p_font,
                          std::unordered_map<std::string, sf::Texture>& p_icons)
        : m_window(p_window), m_font(p_font), m_icons(p_icons)
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Reset the tree renderer.
    // ------------------------------------------------------------------------
    void reset();

    // ------------------------------------------------------------------------
    //! \brief Process a received message.
    //! \param[in] data Message data.
    //! \return True if the message was processed, false otherwise.
    // ------------------------------------------------------------------------
    bool handleMessage(const std::vector<uint8_t>& data);

    // ------------------------------------------------------------------------
    //! \brief Center the camera on the window.
    //! \param[in] p_target Render target.
    // ------------------------------------------------------------------------
    void centerCamera(sf::RenderTarget& p_target);

    // ------------------------------------------------------------------------
    //! \brief Get the camera view.
    //! \return The camera view.
    // ------------------------------------------------------------------------
    const sf::View& getCamera() const
    {
        return m_camera;
    }

    // ------------------------------------------------------------------------
    //! \brief Calculate node positions.
    // ------------------------------------------------------------------------
    void calculateNodePositions();

private:

    // ------------------------------------------------------------------------
    //! \brief Draw method called by SFML.
    //! \param[in] p_target Render target.
    //! \param[in] p_states Render states.
    // ------------------------------------------------------------------------
    void draw(sf::RenderTarget& p_target,
              sf::RenderStates p_states) const override;

    // ------------------------------------------------------------------------
    //! \brief Process a YAML node and create the nodes.
    //! \param[in] node YAML node.
    //! \param[in,out] next_id Next available ID.
    // ------------------------------------------------------------------------
    void createNodes(const YAML::Node& node, uint32_t& next_id);

    // ------------------------------------------------------------------------
    //! \brief Set node icon.
    //! \param[in] p_nodeShape Node to configure.
    //! \param[in] p_name Node name.
    // ------------------------------------------------------------------------
    void setNodeIcon(NodeShape* p_nodeShape, const char* p_name) const;

    // ------------------------------------------------------------------------
    //! \brief Draw a node.
    //! \param[in] p_node Node.
    //! \param[in] p_target Render target.
    //! \param[in] p_states Render states.
    // ------------------------------------------------------------------------
    void drawNode(NodeInfo& p_node,
                  sf::RenderTarget& p_target,
                  sf::RenderStates p_states) const;

    // ------------------------------------------------------------------------
    //! \brief Draw a connection between two nodes.
    //! \param[in] p_start Start position.
    //! \param[in] p_end End position.
    //! \param[in] p_target Render target.
    // ------------------------------------------------------------------------
    void drawConnection(sf::Vector2f p_start,
                        sf::Vector2f p_end,
                        sf::RenderTarget& p_target) const;

    // ------------------------------------------------------------------------
    //! \brief Get color based on status.
    //! \param[in] p_status Node status.
    //! \return Corresponding color.
    // ------------------------------------------------------------------------
    sf::Color getStatusColor(bt::Status p_status) const;

    // ------------------------------------------------------------------------
    //! \brief Debug method to print node information.
    // ------------------------------------------------------------------------
    void debugPrintNodes() const;

private:

    //! \brief Reference to the SFML window
    sf::RenderWindow& m_window;
    //! \brief Font
    sf::Font& m_font;
    //! \brief Icons
    std::unordered_map<std::string, sf::Texture>& m_icons;
    //! \brief Camera
    sf::View m_camera;
    //! \brief Map of nodes indexed by their ID
    std::unordered_map<uint32_t, std::unique_ptr<NodeInfo>> m_nodes;
};

} // namespace bt