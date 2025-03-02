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

#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include "TreeRenderer.hpp"
#include "NodeShape.hpp"

#include <yaml-cpp/yaml.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <algorithm> // Pour std::min et std::max
#include <limits>    // Pour std::numeric_limits

namespace bt {

// ----------------------------------------------------------------------------
void TreeRenderer::reset()
{
    m_nodes.clear();
    m_tree_received = false;
}

// ----------------------------------------------------------------------------
void TreeRenderer::draw(sf::RenderTarget& p_target, sf::RenderStates /*p_states*/) const
{
    // Do not draw if the tree has not been received
    if (!m_tree_received)
    {
        return;
    }

    // Find the root (node without parent)
    auto root = std::find_if(m_nodes.begin(), m_nodes.end(),
        [](const auto& pair)
        {
            return pair.second.parent_id == uint32_t(-1);
        });

    // Draw the tree if a root has been found
    if (root != m_nodes.end())
    {
        // Draw the nodes first
        for (const auto& [id, node] : m_nodes)
        {
            renderNode(node.name.c_str(), node.status, node.position, p_target);
        }

        // Then draw the connections
        for (const auto& [id, node] : m_nodes)
        {
            for (uint32_t childId : node.children_ids)
            {
                auto childIt = m_nodes.find(childId);
                if (childIt != m_nodes.end())
                {
                    drawConnection(node.position, childIt->second.position, p_target);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::handleMessage(const std::vector<uint8_t>& data)
{
    // Ignore empty messages
    if (data.empty())
    {
        return;
    }

    // Determine the message type
    auto const type = static_cast<BehaviorTreeVisualizer::MessageType>(data[0]);

    // Process according to the message type
    if (type == BehaviorTreeVisualizer::MessageType::TREE_STRUCTURE)
    {
        // Deserialize the tree structure in YAML format
        try
        {
            // Clear the existing nodes
            m_nodes.clear();
            size_t offset = 1;

            // Read the length of the YAML string
            uint32_t yaml_length;
            std::memcpy(&yaml_length, data.data() + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            // Check that the length is valid. Max 10 Mo
            if (yaml_length == 0 || yaml_length > 10 * 1024 * 1024)
            {
                std::cerr << "Invalid YAML length: " << yaml_length << std::endl;
                return;
            }

            // Check that the data is complete
            if (offset + yaml_length > data.size())
            {
                std::cerr << "Incomplete YAML data" << std::endl;
                return;
            }

            // Read the YAML string
            std::string yaml_str(reinterpret_cast<const char*>(data.data() + offset), yaml_length);

            std::cout << "=== Received Tree Structure (YAML) ===" << std::endl;
            std::cout << yaml_str << std::endl;
            std::cout << "=========================" << std::endl;

            // Analyze the YAML
            YAML::Node root = YAML::Load(yaml_str);
            YAML::Node tree = root["behavior_tree"];
            if (tree)
            {
                // Process the tree structure
                processYAMLNode(tree, uint32_t(-1));
                m_tree_received = true;
                std::cout << "Total nodes: " << m_nodes.size() << std::endl;
                
                // Calculate the layout once after receiving the tree
                calculateTreeLayout();
            }
            else
            {
                std::cerr << "Invalid YAML structure: 'behavior_tree' node missing" << std::endl;
            }
        } catch (const YAML::Exception& e) {
            std::cerr << "YAML analysis error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception when processing the tree structure: " << e.what() << std::endl;
        }
    }
    else if (type == BehaviorTreeVisualizer::MessageType::STATE_UPDATE)
    {
        // Update the nodes states
        try 
        {
            std::cout << "=== Received State Update ===" << std::endl;

            // Check that the message has a minimum size
            if (data.size() < 1 + sizeof(uint32_t))
            {
                std::cerr << "State update message too short" << std::endl;
                return;
            }

            size_t offset = 1;

            // Read the number of updates
            uint32_t count;
            std::memcpy(&count, data.data() + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            // Check that the number of updates is reasonable
            if (count > 10000) { // Arbitrary limit
                std::cerr << "Too many updates: " << count << std::endl;
                return;
            }

            // Check that the data is complete
            size_t expected_size = offset + count * (sizeof(uint32_t) + sizeof(uint8_t));
            if (expected_size > data.size()) {
                std::cerr << "Incomplete update data" << std::endl;
                return;
            }

            std::cout << "Update of " << count << " nodes" << std::endl;

            // Read each update
            for (uint32_t i = 0; i < count; ++i)
            {
                uint32_t id;
                uint8_t status_value;

                // Read the node ID
                std::memcpy(&id, data.data() + offset, sizeof(uint32_t));
                offset += sizeof(uint32_t);

                // Read the status
                status_value = data[offset];
                offset += sizeof(uint8_t);

                // Update the node status
                if (m_nodes.find(id) != m_nodes.end())
                {
                    m_nodes[id].status = bt::Status(status_value);
                    std::cout << "Node ID: " << id
                              << ", Name: " << m_nodes[id].name
                              << ", New Status: " << static_cast<int>(status_value)
                              << std::endl;
                }
                else
                {
                    std::cerr << "Unknown node ID: " << id << std::endl;
                }
            }
            std::cout << "=========================" << std::endl;
            
            // No need to recalculate layout for status updates
        } catch (const std::exception& e) {
            std::cerr << "Exception when processing the state update: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "Unknown message type: " << static_cast<int>(type) << std::endl;
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::processYAMLNode(const YAML::Node& node, uint32_t parent_id)
{
    // Use a static ID to assign unique IDs to nodes
    static uint32_t next_id = 0;

    // Each node in the YAML is a map with a single key (node type) and a value (node properties)
    for (const auto& item : node)
    {
        std::string node_type = item.first.as<std::string>();
        YAML::Node properties = item.second;

        // Create the node information
        NodeInfo info;
        info.parent_id = parent_id;
        info.status = bt::Status(0); // INVALID
        info.position = {0, 0};

        // Get the node name if available
        if (properties["name"]) {
            info.name = properties["name"].as<std::string>();
        } else {
            info.name = node_type;
        }

        // Assign an ID to this node
        uint32_t current_id = next_id++;
        m_nodes[current_id] = info;

        std::cout << "Added node: " << info.name << " (ID: " << current_id
                  << ", Parent: " << parent_id << ")" << std::endl;

        // Process the children if they are available (for composite nodes)
        if (properties["children"])
        {
            for (const auto& child : properties["children"])
            {
                // Add this child to the parent's children list
                m_nodes[current_id].children_ids.push_back(next_id);
                // Process the child node
                processYAMLNode(child, current_id);
            }
        }

        // Process the unique child if it is available (for decorators)
        if (properties["child"])
        {
            // Add this child to the parent's children list
            m_nodes[current_id].children_ids.push_back(next_id);
            // Process the child node
            processYAMLNode(properties["child"], current_id);
        }
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::calculateTreeLayout()
{
    // If no node is present, do nothing
    if (m_nodes.empty())
    {
        return;
    }

    // Find the root (node without parent)
    auto root = std::find_if(m_nodes.begin(), m_nodes.end(),
        [](const auto& pair) { return pair.second.parent_id == uint32_t(-1); });

    // Do nothing if no root is found
    if (root == m_nodes.end())
    {
        return;
    }

    // Structure to store the layout information
    struct LayoutInfo
    {
        float x;           // Node position x
        float width;       // Total width of the subtree
        float node_width;  // Width of the node
        float node_height; // Height of the node
    };

    std::unordered_map<uint32_t, LayoutInfo> layout;

    // Calculate the dimensions of each node based on its content
    for (auto& [id, node] : m_nodes)
    {
        // Create a temporary node shape to calculate dimensions
        NodeShape tempNode;
        tempNode.setText(node.name.c_str(), m_font, 20);
        tempNode.setPadding(15.0f, 12.0f);
        
        // Get the actual dimensions from the node
        sf::Vector2f dimensions = tempNode.getDimensions();
        
        // Store the node dimensions
        layout[id].node_width = dimensions.x;
        layout[id].node_height = dimensions.y;
    }

    // Recursive function to calculate the positions with infix traversal
    std::function<LayoutInfo(uint32_t, float, float)> calculatePositions;
    calculatePositions = [&](uint32_t node_id, float y, float x_offset) -> LayoutInfo
    {
        auto& node = m_nodes[node_id];
        auto& node_layout = layout[node_id];
        float node_width = node_layout.node_width;
        float node_height = node_layout.node_height;

        // Leaf node (without children)
        if (node.children_ids.empty())
        {
            node.position = { x_offset + node_width / 2.0f, y };
            return { x_offset + node_width / 2.0f, node_width, node_width, node_height };
        }

        // Infix traversal for nodes with children
        std::vector<LayoutInfo> child_layouts;
        float total_width = 0.0f;
        float current_x = x_offset;

        // Calculate the layout of the children
        for (uint32_t child_id : node.children_ids)
        {
            auto child_layout = calculatePositions(child_id,
                y + node_height + VERTICAL_SPACING,
                current_x);

            child_layouts.push_back(child_layout);
            total_width += child_layout.width;
            current_x += child_layout.width + HORIZONTAL_SPACING;
        }

        // Add the horizontal spacing between the children
        if (!node.children_ids.empty()) {
            total_width += HORIZONTAL_SPACING * float(node.children_ids.size() - 1);
        }

        // Use the maximum between the node width and the total width of the children
        total_width = std::max(node_width, total_width);

        // Center the parent node above the children
        float center_x = x_offset + total_width / 2.0f;
        node.position = { center_x, y };

        return { center_x, total_width, node_width, node_height };
    };

    // Calculate the layout from the root
    calculatePositions(root->first, 0, 0);

    // After the layout calculation, adjust the camera to show the entire tree
    if (m_tree_received && !m_nodes.empty())
    {
        // Find the limits of the tree
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();

        for (const auto& [id, node] : m_nodes)
        {
            float half_width = layout[id].node_width / 2.0f;
            float half_height = layout[id].node_height / 2.0f;
            minX = std::min<float>(minX, node.position.x - half_width);
            maxX = std::max<float>(maxX, node.position.x + half_width);
            minY = std::min<float>(minY, node.position.y - half_height);
            maxY = std::max<float>(maxY, node.position.y + half_height);
        }

        // Add a margin
        float padding = 50.0f;
        sf::Vector2f center((minX + maxX) / 2.0f, (minY + maxY) / 2.0f);
        sf::Vector2f size(maxX - minX + padding * 2, maxY - minY + padding * 2);

        // Update the camera
        m_camera.setCenter(center);
        m_camera.setSize(size);
        m_window.setView(m_camera);
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::renderNode(const char* p_name, bt::Status p_status,
                              sf::Vector2f p_position, sf::RenderTarget& p_target) const
{
    // Create a unique key for this node
    std::string nodeKey = std::string(p_name) + "_" + std::to_string(static_cast<int>(p_status));
    
    // Get or create the node from cache
    NodeShape* nodeShape = nullptr;
    auto it = m_nodeShapeCache.find(nodeKey);
    
    if (it == m_nodeShapeCache.end())
    {
        // Create a new node and add it to the cache
        nodeShape = new NodeShape();
        configureNodeShape(nodeShape, p_name, p_status);
        m_nodeShapeCache[nodeKey] = nodeShape;
    }
    else
    {
        // Use the existing node from cache
        nodeShape = it->second;
    }
    
    // Position and draw the node
    drawNodeWithShadow(nodeShape, p_position, p_target);
}

// ----------------------------------------------------------------------------
void TreeRenderer::configureNodeShape(NodeShape* p_nodeShape, const char* p_name, bt::Status p_status) const
{
    // Configure the node
    p_nodeShape->setCornerRadius(10.0f);
    
    // Define colors with gradient
    sf::Color mainColor = getStatusColor(p_status);
    sf::Color secondaryColor = sf::Color(
        sf::Uint8(std::max(0, mainColor.r - 50)),
        sf::Uint8(std::max(0, mainColor.g - 50)),
        sf::Uint8(std::max(0, mainColor.b - 50)),
        mainColor.a
    );
    
    // Apply colors with gradient and border
    p_nodeShape->setColors(mainColor, secondaryColor, sf::Color(0, 255, 255, 200));
    
    // Set text with internal font
    p_nodeShape->setText(p_name, m_font, 20);
    
    // Set icon if available
    setNodeIcon(p_nodeShape, p_name);
    
    // Set internal margins and enable anti-aliasing
    p_nodeShape->setPadding(15.0f, 12.0f);
    p_nodeShape->setTextSmoothing(true);
}

// ----------------------------------------------------------------------------
void TreeRenderer::setNodeIcon(NodeShape* p_nodeShape, const char* p_name) const
{
    if (m_icons.empty())
        return;
        
    // Try to find an icon matching the node name
    std::string nodeName(p_name);
    auto iconIt = m_icons.find(nodeName);
    
    if (iconIt != m_icons.end())
    {
        // Use the matching icon
        p_nodeShape->setIcon(iconIt->second);
    }
    else if (!m_icons.empty())
    {
        // Use the first available icon as default
        p_nodeShape->setIcon(m_icons.begin()->second);
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::drawNodeWithShadow(NodeShape* p_nodeShape, sf::Vector2f p_position, 
                                     sf::RenderTarget& p_target) const
{
    // Position the node
    sf::Vector2f dimensions = p_nodeShape->getDimensions();
    p_nodeShape->setPosition(p_position.x - dimensions.x / 2, p_position.y - dimensions.y / 2);
    
    // Configure blend mode for smoother rendering
    sf::RenderStates states;
    states.blendMode = sf::BlendMode(sf::BlendMode::SrcAlpha, sf::BlendMode::OneMinusSrcAlpha);
    
    // Draw a light shadow first
    NodeShape shadow = *p_nodeShape;
    shadow.setPosition(p_nodeShape->getPosition() + sf::Vector2f(3.0f, 3.0f));
    shadow.setColors(sf::Color(0, 0, 0, 50), sf::Color(0, 0, 0, 20), sf::Color(0, 0, 0, 0));
    p_target.draw(shadow, states);
    
    // Draw the node
    p_target.draw(*p_nodeShape, states);
}

// ----------------------------------------------------------------------------
void TreeRenderer::drawConnection(sf::Vector2f p_start, sf::Vector2f p_end,
                                  sf::RenderTarget& p_target) const
{
    // Create a temporary node shape to get dimensions
    NodeShape tempNodeStart, tempNodeEnd;
    
    // Find the node information for start and end points
    auto findNodeByPosition = [this](const sf::Vector2f& pos) -> const NodeInfo* {
        for (const auto& [id, node] : m_nodes) {
            if (node.position == pos) {
                return &node;
            }
        }
        return nullptr;
    };
    
    const NodeInfo* startNode = findNodeByPosition(p_start);
    const NodeInfo* endNode = findNodeByPosition(p_end);
    
    // Adjust start and end points based on actual node dimensions
    sf::Vector2f start = p_start;
    sf::Vector2f end = p_end;
    
    if (startNode) {
        tempNodeStart.setText(startNode->name.c_str(), m_font, 20);
        tempNodeStart.setPadding(15.0f, 12.0f);
        sf::Vector2f startDim = tempNodeStart.getDimensions();
        start.y += startDim.y / 2.0f; // Bottom of parent node
    }
    
    if (endNode) {
        tempNodeEnd.setText(endNode->name.c_str(), m_font, 20);
        tempNodeEnd.setPadding(15.0f, 12.0f);
        sf::Vector2f endDim = tempNodeEnd.getDimensions();
        end.y -= endDim.y / 2.0f; // Top of child node
    }

    // Create and configure the arc shape
    ArcShape arc;
    arc.setPoints(start, end);
    arc.setColor(sf::Color(0, 200, 200, 255));
    arc.setThickness(3.0f);
    arc.setSegments(60);
    arc.enableConnectionPoints(true);
    arc.setConnectionPointRadius(5.0f);
    arc.setControlPointFactor(0.5f);
    
    // Configure blend mode
    sf::RenderStates states;
    states.blendMode = sf::BlendMode(sf::BlendMode::SrcAlpha, sf::BlendMode::OneMinusSrcAlpha);
    
    // Draw the arc
    p_target.draw(arc, states);
}

// ----------------------------------------------------------------------------
sf::Color TreeRenderer::getStatusColor(bt::Status p_status) const
{
    switch (p_status)
    {
        case bt::Status::SUCCESS:
            return sf::Color(0, 255, 0, 230);      // GREEN with slight transparency
        case bt::Status::FAILURE:
            return sf::Color(255, 0, 0, 230);      // RED with slight transparency
        case bt::Status::RUNNING:
            return sf::Color(255, 255, 0, 230);    // YELLOW with slight transparency
        default:
            return sf::Color(211, 211, 211, 230);  // LIGHTGRAY with slight transparency
    }
}

} // namespace bt
