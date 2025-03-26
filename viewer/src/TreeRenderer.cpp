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

#include "BehaviorTree/Visualizer.hpp"
#include "BehaviorTree/private/Serialization.hpp"

#include "NodeShape.hpp"
#include "TreeRenderer.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm> // Pour std::min et std::max
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits> // Pour std::numeric_limits

namespace bt {

// ----------------------------------------------------------------------------
void TreeRenderer::reset()
{
    m_nodes.clear();
}

// ----------------------------------------------------------------------------
bool TreeRenderer::handleMessage(std::vector<uint8_t> const& data)
{
    // Ignore empty messages
    if (data.empty())
        return false;

    // Deserialize the message
    try
    {
        bt::Deserializer deserializer(data);

        // Read the message type
        uint8_t type_value;
        deserializer >> type_value;
        auto type = static_cast<Visualizer::MessageType>(type_value);

        // Process according to the message type
        if (type == Visualizer::MessageType::TREE_STRUCTURE)
        {
            // Clear the existing nodes
            m_nodes.clear();

            // Read the tree structure YAML string
            std::string yaml_str;
            deserializer >> yaml_str;
            YAML::Node root = YAML::Load(yaml_str);

            // Process the root node (behavior_tree)
            if (root["behavior_tree"])
            {
                uint32_t next_id = 0;
                createNodes(root["behavior_tree"], next_id);
                calculateNodePositions();
                centerCamera(m_window);
                debugPrintNodes();
            }
            else
            {
                std::cerr << "No behavior_tree root node found in YAML"
                          << std::endl;
                return false;
            }
        }
        else if (type == Visualizer::MessageType::STATE_UPDATE)
        {
            // Read the number of states
            uint32_t count;
            deserializer >> count;

            // Process each state update
            for (uint32_t i = 0; i < count; ++i)
            {
                // Read the node ID and status
                uint32_t node_id;
                uint8_t status_value;
                deserializer >> node_id >> status_value;
                bt::Status status = static_cast<bt::Status>(status_value);

                // Update the node status if the node exists
                auto it = m_nodes.find(node_id);
                if (it != m_nodes.end())
                {
                    it->second->status = status;
                    std::cout << "Node ID " << node_id << " "
                              << it->second->name << " updated with status "
                              << bt::to_string(status) << std::endl;
                }
                else
                {
                    std::cerr << "Node ID " << node_id << " not found"
                              << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "Unknown message type: " << static_cast<int>(type)
                      << std::endl;
            return false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception processing YAML tree: " << e.what()
                  << std::endl;
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------
void TreeRenderer::createNodes(YAML::Node const& yaml_node, uint32_t& next_id)
{
    for (auto const& item : yaml_node)
    {
        std::string node_type = item.first.as<std::string>();
        YAML::Node properties = item.second;

        // Store the node with its ID
        uint32_t node_id = next_id++;

        // Create the node information
        m_nodes[node_id] = std::make_unique<NodeInfo>();
        NodeInfo& node = *(m_nodes[node_id].get());
        node.id = node_id;
        node.parent = nullptr;
        node.status = bt::INVALID_STATUS;

        // Handle node name based on properties type
        if (properties.IsMap())
        {
            node.name = properties["name"]
                            ? properties["name"].as<std::string>()
                            : node_type;
        }
        else if (properties.IsScalar())
        {
            node.name = properties.as<std::string>();
        }
        else
        {
            node.name = node_type;
        }

        // Create the node shape
        node.shape = std::make_unique<NodeShape>();
        node.shape->setText(node.name, m_font, 24);
        node.shape->setPadding(20.0f, 15.0f);
        node.shape->setCornerRadius(10.0f);
        node.shape->setTextSmoothing(false);
        setNodeIcon(node.shape.get(), node.name, 0.25f);

        // Process the children if they are available (for composite nodes)
        if (properties.IsMap() && properties["children"])
        {
            for (auto const& child : properties["children"])
            {
                uint32_t child_id = next_id;
                createNodes(child, next_id);
                NodeInfo* child_node = m_nodes[child_id].get();
                child_node->parent = &node;
                node.children.push_back(child_node);
            }
        }

        // Process the unique child if it is available (for decorators)
        if (properties.IsMap() && properties["child"])
        {
            uint32_t child_id = next_id;
            createNodes(properties["child"], next_id);
            NodeInfo* child_node = m_nodes[child_id].get();
            child_node->parent = &node;
            node.children.push_back(child_node);
        }
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::calculateNodePositions()
{
    const float HORIZONTAL_SPACING = 100.0f;
    const float VERTICAL_SPACING = 150.0f;
    const float INITIAL_Y = 100.0f;

    // Get the root node (first node created)
    if (m_nodes.empty())
    {
        return;
    }

    NodeInfo* root = m_nodes[0].get();

    // Position the root at the center horizontally of the window
    root->position = sf::Vector2f(
        static_cast<float>(m_window.getSize().x) / 2.0f, INITIAL_Y);

    // Process all nodes level by level
    std::vector<NodeInfo*> currentLevel = {root};
    std::vector<NodeInfo*> nextLevel;
    float currentY = INITIAL_Y;

    while (!currentLevel.empty())
    {
        nextLevel.clear();
        float currentX = currentLevel.front()->position.x;
        float totalWidth = 0.0f;

        // First pass: calculate total width needed for this level
        for (const auto& node : currentLevel)
        {
            if (!node->children.empty())
            {
                float levelWidth = 0.0f;
                for (const auto& child : node->children)
                {
                    levelWidth +=
                        child->shape->getDimensions().x + HORIZONTAL_SPACING;
                }
                levelWidth -= HORIZONTAL_SPACING; // Remove last spacing
                totalWidth = std::max(totalWidth, levelWidth);
            }
        }

        // Second pass: position nodes and their children
        float startX = currentX - (totalWidth / 2.0f);
        for (const auto& node : currentLevel)
        {
            if (!node->children.empty())
            {
                float childrenWidth = 0.0f;
                for (const auto& child : node->children)
                {
                    childrenWidth +=
                        child->shape->getDimensions().x + HORIZONTAL_SPACING;
                }
                childrenWidth -= HORIZONTAL_SPACING;

                float childX = startX + (childrenWidth / 2.0f);
                for (const auto& child : node->children)
                {
                    child->position =
                        sf::Vector2f(childX, currentY + VERTICAL_SPACING);
                    childX +=
                        child->shape->getDimensions().x + HORIZONTAL_SPACING;
                    nextLevel.push_back(child);
                }
                startX += childrenWidth + HORIZONTAL_SPACING;
            }
        }

        currentY += VERTICAL_SPACING;
        currentLevel = nextLevel;
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::centerCamera(sf::RenderTarget& p_target)
{
    if (m_nodes.empty())
    {
        return;
    }

    // Calculate the bounds of the tree
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    // First pass: calculate bounds
    for (const auto& [id, node] : m_nodes)
    {
        sf::Vector2f pos = node->position;
        sf::Vector2f dim = node->shape->getDimensions();

        // Update bounds considering node dimensions
        minX = std::min(minX, pos.x - dim.x / 2.0f);
        maxX = std::max(maxX, pos.x + dim.x / 2.0f);
        minY = std::min(minY, pos.y - dim.y / 2.0f);
        maxY = std::max(maxY, pos.y + dim.y / 2.0f);
    }

    // Add padding
    const float padding = 100.0f;
    minX -= padding;
    maxX += padding;
    minY -= padding;
    maxY += padding;

    // Calculate the center and size of the tree
    sf::Vector2f center((minX + maxX) / 2.0f, (minY + maxY) / 2.0f);
    sf::Vector2f size(maxX - minX, maxY - minY);

    // Get the window size
    sf::Vector2u windowSize = m_window.getSize();
    float windowRatio =
        static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    float treeRatio = size.x / size.y;

    // Adjust the view size to fit the tree while maintaining aspect ratio
    if (treeRatio > windowRatio)
    {
        // Tree is wider than window, adjust height
        size.y = size.x / windowRatio;
    }
    else
    {
        // Tree is taller than window, adjust width
        size.x = size.y * windowRatio;
    }

    // Set the camera view
    m_camera.setSize(size);
    m_camera.setCenter(center);

    // Apply the view to the target
    p_target.setView(m_camera);

    // Debug output
    std::cout << "Camera bounds: " << std::endl
              << "  minX: " << minX << std::endl
              << "  maxX: " << maxX << std::endl
              << "  minY: " << minY << std::endl
              << "  maxY: " << maxY << std::endl
              << "  center: (" << center.x << ", " << center.y << ")"
              << std::endl
              << "  size: (" << size.x << ", " << size.y << ")" << std::endl;
}

// ----------------------------------------------------------------------------
void TreeRenderer::debugPrintNodes() const
{
    std::cout << "\n=== Debug Nodes Information ===" << std::endl;
    for (const auto& [id, node] : m_nodes)
    {
        sf::Vector2f dim = node->shape->getDimensions();
        std::cout << "Node ID: " << node->id << std::endl
                  << "  Name: " << node->name << std::endl
                  << "  Size: " << dim.x << "x" << dim.y << std::endl
                  << "  Position: (" << node->position.x << ", "
                  << node->position.y << ")" << std::endl
                  << "  Parent: "
                  << (node->parent ? node->parent->name : "none")
                  << " ID: " << (node->parent ? node->parent->id : -1)
                  << std::endl
                  << "  Children count: " << node->children.size() << std::endl;
        if (!node->children.empty())
        {
            std::cout << "  Children: ";
            for (const auto& child : node->children)
            {
                std::cout << child->name << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "-------------------" << std::endl;
    }
    std::cout << "===========================\n" << std::endl;
}

// ----------------------------------------------------------------------------
void TreeRenderer::setNodeIcon(NodeShape* p_node_shape,
                               const std::string& p_name,
                               float p_scale) const
{
    return;

    // Try to find an icon matching the node name
    auto icon_it = m_icons.find(p_name);
    if (icon_it != m_icons.end())
    {
        // Use the matching icon with a smaller scale
        p_node_shape->setIcon(icon_it->second, p_scale);
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::draw(sf::RenderTarget& p_target,
                        sf::RenderStates p_states) const
{
    // Draw the nodes first
    for (auto const& [id, node] : m_nodes)
    {
        drawNode(*node.get(), p_target, p_states);
    }

    // Then draw the connections
    for (auto const& [id, node] : m_nodes)
    {
        for (auto const& child : node->children)
        {
            drawConnection(node->position, child->position, p_target);
        }
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::drawNode(NodeInfo& p_node,
                            sf::RenderTarget& p_target,
                            sf::RenderStates p_states) const
{
    NodeShape& shape = *(p_node.shape.get());

    // Define the color status with gradient
    sf::Color main_color = getStatusColor(p_node.status);
    sf::Color secondary_color =
        sf::Color(sf::Uint8(std::max(0, main_color.r - 50)),
                  sf::Uint8(std::max(0, main_color.g - 50)),
                  sf::Uint8(std::max(0, main_color.b - 50)),
                  main_color.a);
    shape.setColors(main_color, secondary_color, sf::Color(0, 255, 255, 200));

    // Set the position of the shape, taking into account the dimensions for
    // centering
    sf::Vector2f dim = shape.getDimensions();
    shape.setPosition(p_node.position.x - dim.x / 2.0f,
                      p_node.position.y - dim.y / 2.0f);
    p_target.draw(shape, p_states);
}

// ----------------------------------------------------------------------------
void TreeRenderer::drawConnection(sf::Vector2f p_start,
                                  sf::Vector2f p_end,
                                  sf::RenderTarget& p_target) const
{
    // Find the node information for start and end points
    auto findNodeByPosition =
        [this](const sf::Vector2f& pos) -> const NodeInfo* {
        for (auto const& [id, node] : m_nodes)
        {
            if (node->position == pos)
            {
                return node.get();
            }
        }
        return nullptr;
    };

    const NodeInfo* start_node = findNodeByPosition(p_start);
    const NodeInfo* end_node = findNodeByPosition(p_end);

    if (!start_node || !end_node)
    {
        return;
    }

    // Get the dimensions of both nodes
    sf::Vector2f start_dim = start_node->shape->getDimensions();
    sf::Vector2f end_dim = end_node->shape->getDimensions();

    // Calculate the connection points at the center bottom of parent and center
    // top of child
    sf::Vector2f start = p_start;
    start.y += start_dim.y / 2.0f; // Bottom center of parent node

    sf::Vector2f end = p_end;
    end.y -= end_dim.y / 2.0f; // Top center of child node

    // Calculate horizontal distance between nodes
    float horizontal_distance = std::abs(end.x - start.x);
    float vertical_distance = end.y - start.y;

    // Adjust control point factor based on horizontal distance
    float control_factor = 0.5f; // Default value
    if (horizontal_distance < vertical_distance * 0.5f)
    {
        // If nodes are close horizontally, increase the curve
        control_factor = 0.8f;
    }
    else if (horizontal_distance < vertical_distance)
    {
        // For medium distances, use a moderate curve
        control_factor = 0.6f;
    }

    // Create and configure the arc shape
    ArcShape arc;
    arc.setPoints(start, end);
    arc.setColor(sf::Color(0, 200, 200, 255));
    arc.setThickness(3.0f);
    arc.setSegments(60);
    arc.enableConnectionPoints(true);
    arc.setConnectionPointRadius(4.0f);
    arc.setControlPointFactor(control_factor);

    // Configure blend mode
    sf::RenderStates states;
    states.blendMode =
        sf::BlendMode(sf::BlendMode::SrcAlpha, sf::BlendMode::OneMinusSrcAlpha);

    // Draw the arc
    p_target.draw(arc, states);
}

// ----------------------------------------------------------------------------
sf::Color TreeRenderer::getStatusColor(bt::Status p_status) const
{
    switch (p_status)
    {
        case bt::Status::SUCCESS:
            return sf::Color(0, 255, 0, 230); // GREEN with slight transparency
        case bt::Status::FAILURE:
            return sf::Color(255, 0, 0, 230); // RED with slight transparency
        case bt::Status::RUNNING:
            return sf::Color(
                255, 255, 0, 230); // YELLOW with slight transparency
        default:
            return sf::Color(
                211, 211, 211, 230); // LIGHTGRAY with slight transparency
    }
}

} // namespace bt
