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

#pragma once

#include "NodeShape.hpp"
#include "ArcShape.hpp"
#include "BehaviorTree/BehaviorTreeVisualizer.hpp"

#include <SFML/Graphics.hpp>

#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

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
class TreeRenderer : public sf::Drawable
{
public:
    //! \brief Constant for rendering
    static constexpr float HORIZONTAL_SPACING = 100.0f;
    static constexpr float VERTICAL_SPACING = 120.0f;

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] p_window Reference to the SFML window.
    //! \param[in] p_font Reference to the SFML font.
    //! \param[in] p_icons Reference to the SFML icons.
    // ------------------------------------------------------------------------
    explicit TreeRenderer(sf::RenderWindow& p_window, sf::Font& p_font,
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
    // ------------------------------------------------------------------------
    void handleMessage(const std::vector<uint8_t>& data);

private:
    // ------------------------------------------------------------------------
    //! \brief Draw method called by SFML.
    //! \param[in] p_target Render target.
    //! \param[in] p_states Render states.
    // ------------------------------------------------------------------------
    void draw(sf::RenderTarget& p_target, sf::RenderStates p_states) const override;

    // ------------------------------------------------------------------------
    //! \brief Process a YAML node.
    //! \param[in] node YAML node.
    //! \param[in] parent_id Parent ID.
    // ------------------------------------------------------------------------
    void processYAMLNode(const YAML::Node& node, uint32_t parent_id);

    // ------------------------------------------------------------------------
    //! \brief Calculate the tree layout (positions of nodes).
    // ------------------------------------------------------------------------
    void calculateTreeLayout();

    // ------------------------------------------------------------------------
    //! \brief Render a node.
    //! \param[in] p_name Node name.
    //! \param[in] p_status Node status.
    //! \param[in] p_position Node position.
    //! \param[in] p_target Render target.
    // ------------------------------------------------------------------------
    void renderNode(const char* p_name, bt::Status p_status,
                    sf::Vector2f p_position, sf::RenderTarget& p_target) const;

    // ------------------------------------------------------------------------
    //! \brief Configure a node.
    //! \param[in] p_nodeShape Node to configure.
    //! \param[in] p_name Node name.
    //! \param[in] p_status Node status.
    // ------------------------------------------------------------------------
    void configureNodeShape(NodeShape* p_nodeShape, const char* p_name, bt::Status p_status) const;

    // ------------------------------------------------------------------------
    //! \brief Set node icon.
    //! \param[in] p_nodeShape Node to configure.
    //! \param[in] p_name Node name.
    // ------------------------------------------------------------------------
    void setNodeIcon(NodeShape* p_nodeShape, const char* p_name) const;

    // ------------------------------------------------------------------------
    //! \brief Draw a node with shadow.
    //! \param[in] p_nodeShape Node to draw.
    //! \param[in] p_position Node position.
    //! \param[in] p_target Render target.
    // ------------------------------------------------------------------------
    void drawNodeWithShadow(NodeShape* p_nodeShape, sf::Vector2f p_position,
                           sf::RenderTarget& p_target) const;

    // ------------------------------------------------------------------------
    //! \brief Draw a connection between two nodes.
    //! \param[in] p_start Start position.
    //! \param[in] p_end End position.
    //! \param[in] p_target Render target.
    // ------------------------------------------------------------------------
    void drawConnection(sf::Vector2f p_start, sf::Vector2f p_end,
                        sf::RenderTarget& p_target) const;

    // ------------------------------------------------------------------------
    //! \brief Get color based on status.
    //! \param[in] p_status Node status.
    //! \return Corresponding color.
    // ------------------------------------------------------------------------
    sf::Color getStatusColor(bt::Status p_status) const;

    // ------------------------------------------------------------------------
    //! \brief Center the camera on the window.
    //! \param[in] p_target Render target.
    // ------------------------------------------------------------------------
    void centerCamera(sf::RenderTarget& p_target) const;

private:

    // ************************************************************************
    //! \brief Structure to store node information
    // ************************************************************************
    struct NodeInfo
    {
        uint32_t id;                       //!< Node ID
        std::string name;                  //!< Node name
        bt::Status status;                 //!< Node status
        sf::Vector2f position;             //!< Node position
        uint32_t parent_id;                //!< Parent ID
        std::vector<uint32_t> children_ids; //!< Children IDs
        NodeShape* shape;                  //!< Node shape
    };

    //! \brief Reference to the SFML window
    sf::RenderWindow& m_window;
    //! \brief Font
    sf::Font& m_font;
    //! \brief Icon collection
    std::unordered_map<std::string, sf::Texture>& m_icons;
    //! \brief Camera view
    mutable sf::View m_camera;
    //! \brief Tree nodes
    mutable std::vector<NodeInfo> m_nodes;
};

} // namespace bt