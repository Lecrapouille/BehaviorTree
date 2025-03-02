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

namespace {
// ----------------------------------------------------------------------------
//! \brief Count total number of nodes in a YAML tree
//! \param[in] node YAML node to count
//! \return Total number of nodes
// ----------------------------------------------------------------------------
size_t countNodesInYAML(const YAML::Node& node)
{
    size_t count = 0;
    for (const auto& item : node)
    {
        count += 1;
        if (item.second["children"])
        {
            for (const auto& child : item.second["children"])
            {
                count += countNodesInYAML(child);
            }
        }
        if (item.second["child"])
        {
            count += countNodesInYAML(item.second["child"]);
        }
    }
    return count;
}
} // anonymous namespace

// ----------------------------------------------------------------------------
void TreeRenderer::reset()
{
    m_nodes.clear();
}

// ----------------------------------------------------------------------------
void TreeRenderer::draw(sf::RenderTarget& p_target, sf::RenderStates /*p_states*/) const
{
    // Do not draw if the tree has not been received
    if (m_nodes.empty())
    {
        return;
    }

    // Center the camera on the window
    centerCamera(p_target);

    // Find the root (node without parent)
    auto root = std::find_if(m_nodes.begin(), m_nodes.end(),
        [](const NodeInfo& node)
        {
            return node.parent_id == uint32_t(-1);
        });

    // Draw the tree if a root has been found
    if (root != m_nodes.end())
    {
        // Draw the nodes first
        for (const auto& node : m_nodes)
        {
            renderNode(node.name.c_str(), node.status, node.position, p_target);
        }

        // Then draw the connections
        for (const auto& node : m_nodes)
        {
            for (uint32_t childId : node.children_ids)
            {
                auto childIt = std::find_if(m_nodes.begin(), m_nodes.end(),
                    [childId](const NodeInfo& n) { return n.id == childId; });
                if (childIt != m_nodes.end())
                {
                    drawConnection(node.position, childIt->position, p_target);
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

            // Read the number of nodes
            uint32_t count;
            std::memcpy(&count, data.data() + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            std::cout << "Received state update for " << count << " nodes" << std::endl;
            std::cout << "=========================" << std::endl;

            // Process each node
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
                auto nodeIt = std::find_if(m_nodes.begin(), m_nodes.end(),
                    [id](const NodeInfo& node) { return node.id == id; });
                if (nodeIt != m_nodes.end())
                {
                    nodeIt->status = bt::Status(status_value);
                    std::cout << "Node ID: " << id
                              << ", Name: " << nodeIt->name
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
    // Use a static index to track current position in vector
    static uint32_t current_index = 0;

    // First call: reset counter and resize vector
    if (parent_id == uint32_t(-1))
    {
        current_index = 0;
        size_t total_nodes = countNodesInYAML(node);
        m_nodes.resize(total_nodes);
    }

    // Each node in the YAML is a map with a single key (node type) and a value (node properties)
    for (const auto& item : node)
    {
        std::string node_type = item.first.as<std::string>();
        YAML::Node properties = item.second;

        // Store current node index before processing children
        uint32_t node_index = current_index++;

        // Create the node information
        NodeInfo& info = m_nodes[node_index];
        info.id = node_index;  // Use index as ID
        info.parent_id = parent_id;
        info.status = bt::Status(0); // INVALID
        info.position = {0, 0};
        info.shape = nullptr;

        // Get the node name if available
        if (properties["name"]) {
            info.name = properties["name"].as<std::string>();
        } else {
            info.name = node_type;
        }

        std::cout << "Added node: " << info.name << " (ID: " << info.id
                  << ", Parent: " << parent_id << ")" << std::endl;

        // Process the children if they are available (for composite nodes)
        if (properties["children"])
        {
            for (const auto& child : properties["children"])
            {
                // Add this child to the parent's children list
                info.children_ids.push_back(current_index);  // Use current_index as next child's ID
                // Process the child node
                processYAMLNode(child, node_index);
            }
        }

        // Process the unique child if it is available (for decorators)
        if (properties["child"])
        {
            // Add this child to the parent's children list
            info.children_ids.push_back(current_index);  // Use current_index as next child's ID
            // Process the child node
            processYAMLNode(properties["child"], node_index);
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
        [](const NodeInfo& node) { return node.parent_id == uint32_t(-1); });

    // Do nothing if no root is found
    if (root == m_nodes.end())
    {
        return;
    }

    // Structure to store the layout information
    struct LayoutInfo
    {
        uint32_t id;         // Node ID
        float x;             // Node position x
        float width;         // Total width of the subtree
        float node_width;    // Width of the node
        float node_height;   // Height of the node
    };

    std::vector<LayoutInfo> layout;
    layout.reserve(m_nodes.size());  // Pre-allocate to avoid reallocation

    // Calculate the dimensions of each node based on its content
    float maxNodeWidth = 0.0f;
    float maxNodeHeight = 0.0f;

    for (const auto& node : m_nodes)
    {
        // Create a temporary node shape to calculate dimensions
        NodeShape tempNode;
        tempNode.setText(node.name.c_str(), m_font, 20);
        tempNode.setPadding(15.0f, 12.0f);

        // Get the actual dimensions from the node
        sf::Vector2f dimensions = tempNode.getDimensions();

        // Update max dimensions
        maxNodeWidth = std::max(maxNodeWidth, dimensions.x);
        maxNodeHeight = std::max(maxNodeHeight, dimensions.y);

        // Store the node dimensions
        LayoutInfo info;
        info.id = node.id;
        info.node_width = dimensions.x;
        info.node_height = dimensions.y;
        layout.push_back(info);
    }

    // Add extra spacing based on max node dimensions
    float extraHorizontalSpacing = maxNodeWidth * 0.5f;
    float extraVerticalSpacing = maxNodeHeight * 0.5f;

    // First pass: calculate total width needed
    std::function<float(uint32_t)> calculateWidth;
    calculateWidth = [this, &layout, &calculateWidth, extraHorizontalSpacing](uint32_t node_id) -> float {
        auto& node = m_nodes[node_id];
        auto& info = layout[node_id];

        if (node.children_ids.empty())
        {
            return info.node_width + extraHorizontalSpacing;
        }

        float children_width = 0.0f;
        for (uint32_t child_id : node.children_ids)
        {
            children_width += calculateWidth(child_id) + HORIZONTAL_SPACING + extraHorizontalSpacing;
        }
        children_width -= (HORIZONTAL_SPACING + extraHorizontalSpacing); // Remove extra spacing after last child

        return std::max(children_width, info.node_width + extraHorizontalSpacing);
    };

    // Calculate total width needed
    float totalWidth = calculateWidth(root->id);

    // Recursive function to calculate the positions with infix traversal
    std::function<LayoutInfo(uint32_t, float, float)> calculatePositions;
    calculatePositions = [this, &layout, &calculatePositions, extraHorizontalSpacing, extraVerticalSpacing](uint32_t node_id, float x, float level) -> LayoutInfo {
        // Find the current node
        auto& node = m_nodes[node_id];
        auto& info = layout[node_id];
        info.x = x;

        if (node.children_ids.empty())
        {
            info.width = info.node_width + extraHorizontalSpacing;
        }
        else
        {
            float children_width = 0.0f;
            float child_x = x - (node.children_ids.size() * (HORIZONTAL_SPACING + extraHorizontalSpacing)) / 2.0f;

            for (uint32_t child_id : node.children_ids)
            {
                LayoutInfo child_info = calculatePositions(child_id,
                    child_x + children_width,
                    level + 1);

                children_width += child_info.width + HORIZONTAL_SPACING + extraHorizontalSpacing;
            }

            children_width -= (HORIZONTAL_SPACING + extraHorizontalSpacing);
            info.width = std::max(children_width, info.node_width + extraHorizontalSpacing);
            info.x = x + (children_width - info.node_width) / 2.0f;
        }

        // Update the node's position
        node.position = sf::Vector2f(info.x, level * (VERTICAL_SPACING + extraVerticalSpacing));

        return info;
    };

    // Calculate the layout from the root, starting at -totalWidth/2 to center the tree
    calculatePositions(root->id, -totalWidth/2, 0);

    // After the layout calculation, adjust the camera to show the entire tree
    if (!m_nodes.empty())
    {
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();

        // First pass: find the actual bounds of all nodes
        for (const auto& node : m_nodes)
        {
            const auto& layoutInfo = layout[node.id];
            float half_width = layoutInfo.node_width / 2.0f;
            float half_height = layoutInfo.node_height / 2.0f;
            minX = std::min<float>(minX, node.position.x - half_width);
            maxX = std::max<float>(maxX, node.position.x + half_width);
            minY = std::min<float>(minY, node.position.y - half_height);
            maxY = std::max<float>(maxY, node.position.y + half_height);
        }

        // Add padding (10% of dimensions instead of 20%)
        float width = maxX - minX;
        float height = maxY - minY;
        float paddingX = width * 0.1f;
        float paddingY = height * 0.1f;
        minX -= paddingX;
        maxX += paddingX;
        minY -= paddingY;
        maxY += paddingY;

        // Calculate the center and size of the view
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        width = maxX - minX;
        height = maxY - minY;

        // Get the window size and calculate the aspect ratio
        sf::Vector2u windowSize = m_window.getSize();
        float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
        float treeRatio = width / height;

        // Adjust the view size to match the window aspect ratio while keeping the tree visible
        if (treeRatio > windowRatio)
        {
            // Tree is wider than window
            height = width / windowRatio;
        }
        else
        {
            // Tree is taller than window
            width = height * windowRatio;
        }

        // Set the camera view with the adjusted dimensions
        m_camera.setCenter(centerX, centerY);
        m_camera.setSize(width * 0.8f, height * 0.8f); // Zoom in by reducing the view size by 20%
    }
}

// ----------------------------------------------------------------------------
void TreeRenderer::renderNode(const char* p_name, bt::Status p_status,
                              sf::Vector2f p_position, sf::RenderTarget& p_target) const
{
    // Create a new node shape for each render
    NodeShape* nodeShape = new NodeShape();
    configureNodeShape(nodeShape, p_name, p_status);

    // Position and draw the node
    drawNodeWithShadow(nodeShape, p_position, p_target);

    // Clean up
    delete nodeShape;
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

    // Set text with internal font with larger size
    p_nodeShape->setText(p_name, m_font, 24);  // Increased font size

    // Set icon if available
    setNodeIcon(p_nodeShape, p_name);

    // Set larger internal margins and enable anti-aliasing
    p_nodeShape->setPadding(20.0f, 15.0f);  // Increased padding
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
        for (const auto& node : m_nodes) {
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

// ----------------------------------------------------------------------------
void TreeRenderer::centerCamera(sf::RenderTarget& p_target) const
{
    // Set the current view to the camera
    p_target.setView(m_camera);

    // Get the current window size
    sf::Vector2u windowSize = m_window.getSize();

    // Get the current camera center and size
    sf::Vector2f center = m_camera.getCenter();
    sf::Vector2f size = m_camera.getSize();

    // Calculate the aspect ratios
    float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    float viewRatio = size.x / size.y;

    // Adjust the camera size to match the window aspect ratio
    if (viewRatio > windowRatio)
    {
        // View is wider than window, adjust height
        size.y = size.x / windowRatio;
    }
    else
    {
        // View is taller than window, adjust width
        size.x = size.y * windowRatio;
    }

    // Update the camera
    m_camera.setSize(size);
    m_camera.setCenter(center);
}

} // namespace bt
