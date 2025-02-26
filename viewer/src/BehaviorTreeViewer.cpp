#include "BehaviorTreeViewer.hpp"
#include <cstring>
#include <thread>
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace bt {

// ----------------------------------------------------------------------------
BehaviorTreeViewer::BehaviorTreeViewer()
{
    // Initialize the camera to the center of the view
    m_camera.setCenter(0, 0);
    m_camera.setSize(1280, 720);
}

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
    m_window.close();
}

// ----------------------------------------------------------------------------
bool BehaviorTreeViewer::initialize(uint32_t p_width, uint32_t p_height, uint16_t p_port)
{
    // Store the dimensions and port
    m_width = p_width;
    m_height = p_height;
    m_port = p_port;

    // Create the SFML window
    m_window.create(sf::VideoMode(m_width, m_height), "Behavior Tree Viewer");
    m_window.setFramerateLimit(60);

    // Initialize the camera with the default view
    m_camera = m_window.getDefaultView();
    m_window.setView(m_camera);

    // Load the font
    if (!m_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    {
        std::cerr << "Error: Impossible to load the DejaVuSans.ttf font" << std::endl;
        return false;
    }

    // Create the node renderer
    m_renderer = std::make_unique<NodeRenderer>();

    // Initialize the debug server with a callback to process messages
    m_server = std::make_unique<DebugServer>(p_port, [this](const std::vector<uint8_t>& data)
    {
        handleMessage(data);
    });

    // Start the server
    return m_server->start();
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::run()
{
    // Prepare the help text
    sf::Text helpText;
    helpText.setFont(m_font);
    helpText.setString("Waiting for the behavior tree on port " +
        std::to_string(m_port) + "...\nUse middle mouse to pan, Ctrl+wheel to zoom");
    helpText.setCharacterSize(20);
    helpText.setFillColor(sf::Color(64, 64, 64));
    helpText.setPosition(10, 10);

    // Main loop
    while (m_window.isOpen())
    {
        // Process events
        sf::Event event;
        while (m_window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_window.close();
                return;
            }
            else if (event.type == sf::Event::MouseWheelScrolled)
            {
                // Zoom with Ctrl + wheel
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                {
                    float zoom = (event.mouseWheelScroll.delta > 0) ? 0.9f : 1.1f;
                    m_camera.zoom(zoom);
                }
            }
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                // Start panning with the middle button
                if (event.mouseButton.button == sf::Mouse::Middle)
                {
                    m_is_panning = true;
                    m_last_mouse_pos = m_window.mapPixelToCoords(
                        sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased)
            {
                // Stop panning
                if (event.mouseButton.button == sf::Mouse::Middle)
                    m_is_panning = false;
            }
            else if (event.type == sf::Event::MouseMoved)
            {
                // Move the camera during panning
                if (m_is_panning)
                {
                    sf::Vector2f current_pos = m_window.mapPixelToCoords(
                        sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
                    sf::Vector2f delta = m_last_mouse_pos - current_pos;
                    m_camera.move(delta);
                    m_window.setView(m_camera);
                    m_last_mouse_pos = current_pos;
                }
            }
        }

        // Draw the tree
        m_window.clear(sf::Color::White);
        m_window.setView(m_camera);
        draw();

        // Draw the help text if necessary
        m_window.setView(m_window.getDefaultView());
        if ((m_server == nullptr) || (!m_server->isConnected()))
        {
            m_window.draw(helpText);
        }

        // Display the result
        m_window.display();
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::draw()
{
    // Don't draw if the tree has not been received
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
        // Update the nodes layout
        updateLayout();

        // Draw the connections first
        for (const auto& [id, node] : m_nodes)
        {
            for (uint32_t childId : node.children_ids)
            {
                auto childIt = m_nodes.find(childId);
                if (childIt != m_nodes.end())
                {
                    m_renderer->drawConnection(
                        node.position,
                        childIt->second.position,
                        m_window
                    );
                }
            }
        }

        // Then draw the nodes
        for (const auto& [id, node] : m_nodes)
        {
            m_renderer->renderNode(
                node.name.c_str(),
                node.status,
                node.position,
                m_font,
                m_window
            );
        }
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::handleMessage(const std::vector<uint8_t>& data)
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
        try {
            // Clear the existing nodes
            m_nodes.clear();
            size_t offset = 1;

            // Read the length of the YAML string
            uint32_t yaml_length;
            std::memcpy(&yaml_length, data.data() + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            // Check that the length is valid
            if (yaml_length == 0 || yaml_length > 10 * 1024 * 1024) { // Max 10 Mo
                std::cerr << "Invalid YAML length: " << yaml_length << std::endl;
                return;
            }

            // Check that the data is complete
            if (offset + yaml_length > data.size()) {
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

            if (tree) {
                // Process the tree structure
                processYAMLNode(tree, uint32_t(-1));
                m_tree_received = true;
                std::cout << "Total nodes: " << m_nodes.size() << std::endl;
            } else {
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
        try {
            std::cout << "=== Received State Update ===" << std::endl;

            // Check that the message has a minimum size
            if (data.size() < 1 + sizeof(std::time_t) + sizeof(uint32_t)) {
                std::cerr << "State update message too short" << std::endl;
                return;
            }

            size_t offset = 1;

            // Read the timestamp
            std::time_t timestamp;
            std::memcpy(&timestamp, data.data() + offset, sizeof(std::time_t));
            offset += sizeof(std::time_t);

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

            std::cout << "Update of " << count << " nodes at time " << timestamp << std::endl;

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
void BehaviorTreeViewer::processYAMLNode(const YAML::Node& node, uint32_t parent_id)
{
    // Use a static ID to assign unique IDs to nodes
    static uint32_t next_id = 0;

    // Each node in the YAML is a map with a single key (node type) and a value (node properties)
    for (const auto& item : node) {
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
        if (properties["children"]) {
            for (const auto& child : properties["children"]) {
                // Add this child to the parent's children list
                m_nodes[current_id].children_ids.push_back(next_id);
                // Process the child node
                processYAMLNode(child, current_id);
            }
        }

        // Process the unique child if it is available (for decorators)
        if (properties["child"]) {
            // Add this child to the parent's children list
            m_nodes[current_id].children_ids.push_back(next_id);
            // Process the child node
            processYAMLNode(properties["child"], current_id);
        }
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::updateLayout()
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
        float node_width;  // Node width based on its name
    };

    std::unordered_map<uint32_t, LayoutInfo> layout;

    // Calculate the width of each node based on the length of its name
    for (auto& [id, node] : m_nodes)
    {
        // Calculate the minimum width based on the length of the name
        // Use a minimum width for short names
        float name_width = std::max(
            NodeRenderer::NODE_WIDTH,
            float(node.name.length() * 12.0f)  // Estimation of the text width
        );

        // Store the node width
        layout[id].node_width = name_width;
    }

    // Recursive function to calculate the positions with infix traversal
    std::function<LayoutInfo(uint32_t, float, float)> calculatePositions;
    calculatePositions = [&](uint32_t node_id, float y, float x_offset) -> LayoutInfo
    {
        auto& node = m_nodes[node_id];
        auto& node_layout = layout[node_id];
        float node_width = node_layout.node_width;

        // Leaf node (without children)
        if (node.children_ids.empty())
        {
            node.position = { x_offset + node_width / 2.0f, y };
            return { x_offset + node_width / 2.0f, node_width, node_width };
        }

        // Infix traversal for nodes with children
        std::vector<LayoutInfo> child_layouts;
        float total_width = 0.0f;
        float current_x = x_offset;

        // Calculate the layout of the children
        for (uint32_t child_id : node.children_ids)
        {
            auto child_layout = calculatePositions(child_id,
                y + NodeRenderer::NODE_HEIGHT + NodeRenderer::VERTICAL_SPACING,
                current_x);

            child_layouts.push_back(child_layout);
            total_width += child_layout.width;
            current_x += child_layout.width + NodeRenderer::HORIZONTAL_SPACING;
        }

        // Add the horizontal spacing between the children
        if (!node.children_ids.empty()) {
            total_width += NodeRenderer::HORIZONTAL_SPACING * float(node.children_ids.size() - 1);
        }

        // Use the maximum between the node width and the total width of the children
        total_width = std::max(node_width, total_width);

        // Center the parent node above the children
        float center_x = x_offset + total_width / 2.0f;
        node.position = { center_x, y };

        return { center_x, total_width, node_width };
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
            minX = std::min(minX, node.position.x - half_width);
            maxX = std::max(maxX, node.position.x + half_width);
            minY = std::min(minY, node.position.y - NodeRenderer::NODE_HEIGHT/2.0f);
            maxY = std::max(maxY, node.position.y + NodeRenderer::NODE_HEIGHT/2.0f);
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

} // namespace bt