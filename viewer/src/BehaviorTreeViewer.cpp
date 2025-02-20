#include "BehaviorTreeViewer.hpp"
#include <cstring>
#include <thread>
#include <iostream>

namespace bt {

// ----------------------------------------------------------------------------
BehaviorTreeViewer::BehaviorTreeViewer()
{
    m_camera.setCenter(0, 0);
    m_camera.setSize(1280, 720);
}

// ----------------------------------------------------------------------------
BehaviorTreeViewer::~BehaviorTreeViewer()
{
    if (m_server != nullptr)
    {
        m_server->stop();
        m_server = nullptr;
    }
    m_window.close();
}

// ----------------------------------------------------------------------------
bool BehaviorTreeViewer::initialize(uint32_t p_width, uint32_t p_height, uint16_t p_port)
{
    m_width = p_width;
    m_height = p_height;
    m_port = p_port;

    m_window.create(sf::VideoMode(m_width, m_height), "Behavior Tree Viewer");
    m_window.setFramerateLimit(60);

    // Initialize the camera with default view
    m_camera = m_window.getDefaultView();
    m_window.setView(m_camera);

    // Load the font
    if (!m_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    {
        return false;
    }

    // Create the renderer
    m_renderer = std::make_unique<NodeRenderer>();

    // Initialize the debug server
    m_server = std::make_unique<DebugServer>(p_port, [this](const std::vector<uint8_t>& data)
    {
        handleMessage(data);
    });

    return m_server->start();
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::run()
{
    sf::Text helpText;
    helpText.setFont(m_font);
    helpText.setString("Waiting for the behavior tree on port " +
        std::to_string(m_port) + "...\nUse middle mouse to pan, Ctrl+wheel to zoom");
    helpText.setCharacterSize(20);
    helpText.setFillColor(sf::Color(64, 64, 64));
    helpText.setPosition(10, 10);

    while (m_window.isOpen())
    {
        sf::Event event;
        while (m_window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_window.close();
                return ;
            }
            else if (event.type == sf::Event::MouseWheelScrolled)
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                {
                    float zoom = (event.mouseWheelScroll.delta > 0) ? 0.9f : 1.1f;
                    m_camera.zoom(zoom);
                }
            }
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Middle)
                {
                    m_is_panning = true;
                    m_last_mouse_pos = m_window.mapPixelToCoords(
                        sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased)
            {
                if (event.mouseButton.button == sf::Mouse::Middle)
                    m_is_panning = false;
            }
            else if (event.type == sf::Event::MouseMoved)
            {
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

        // Draw the help text
        m_window.setView(m_window.getDefaultView());
        if ((m_server == nullptr) || (!m_server->isConnected()))
        {
            m_window.draw(helpText);
        }

        m_window.display();
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::draw()
{
    if (!m_tree_received)
    {
        return;
    }

    // Find the root
    auto root = std::find_if(m_nodes.begin(), m_nodes.end(),
        [](const auto& pair)
        {
            return pair.second.parent_id == uint32_t(-1);
        });

    if (root != m_nodes.end())
    {
        updateLayout();

        // Draw connections first
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
    if (data.empty())
    {
        return;
    }

    auto const type = static_cast<BehaviorTreeVisualizer::MessageType>(data[0]);
    if (type == BehaviorTreeVisualizer::MessageType::TREE_STRUCTURE)
    {
        // Deserialization of the tree structure
        m_nodes.clear();
        size_t offset = 1;

        std::cout << "=== Received Tree Structure ===" << std::endl;

        while (offset < data.size())
        {
            NodeInfo info;
            uint32_t id;

            // Reading the ID
            std::memcpy(&id, data.data() + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            // Reading the name
            uint16_t name_length;
            std::memcpy(&name_length, data.data() + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);
            info.name = std::string(reinterpret_cast<const char*>(data.data() + offset), name_length);
            offset += name_length;

            // Reading the parent ID
            std::memcpy(&info.parent_id, data.data() + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            // Reading the children IDs
            uint16_t child_count;
            std::memcpy(&child_count, data.data() + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            for (uint16_t i = 0; i < child_count; ++i)
            {
                uint32_t child_id;
                std::memcpy(&child_id, data.data() + offset, sizeof(uint32_t));
                offset += sizeof(uint32_t);
                info.children_ids.push_back(child_id);
            }

            info.status = bt::Status(0); // INVALID
            info.position = {0, 0};
            m_nodes[id] = info;

            // Print node information
            std::cout << "Node ID: " << id 
                      << ", Name: " << info.name 
                      << ", Parent: " << info.parent_id 
                      << ", Children: ";
            for (auto child : info.children_ids) {
                std::cout << child << " ";
            }
            std::cout << std::endl;
        }

        m_tree_received = true;
        std::cout << "Total nodes: " << m_nodes.size() << std::endl;
        std::cout << "=========================" << std::endl;
    }
    else if (type == BehaviorTreeVisualizer::MessageType::STATE_UPDATE)
    {
        std::cout << "=== Received State Update ===" << std::endl;
        // Update the nodes states
        size_t offset = 1;

        while (offset < data.size())
        {
            uint32_t id;
            bt::Status status;

            std::memcpy(&id, data.data() + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            std::memcpy(&status, data.data() + offset, sizeof(bt::Status));
            offset += sizeof(bt::Status);

            if (m_nodes.find(id) != m_nodes.end())
            {
                m_nodes[id].status = status;
                // Print state update
                std::cout << "Node ID: " << id 
                          << ", Name: " << m_nodes[id].name 
                          << ", New Status: " << static_cast<int>(status) 
                          << ", Position: (" << m_nodes[id].position.x << ", " 
                          << m_nodes[id].position.y << ")" << std::endl;
            }
        }
        std::cout << "=========================" << std::endl;
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeViewer::updateLayout()
{
    if (m_nodes.empty())
    {
        return;
    }

    // Find the root
    auto root = std::find_if(m_nodes.begin(), m_nodes.end(),
        [](const auto& pair) { return pair.second.parent_id == uint32_t(-1); });

    if (root == m_nodes.end())
    {
        return;
    }

    // Structure to store the layout information
    struct LayoutInfo { float x; float width; };
    std::unordered_map<uint32_t, LayoutInfo> layout;

    // Recursive function to calculate the positions
    std::function<LayoutInfo(uint32_t, float)> calculatePositions;
    calculatePositions = [&](uint32_t node_id, float y) -> LayoutInfo
    {
        auto& node = m_nodes[node_id];

        if (node.children_ids.empty())
        {
            // Leaf node
            node.position = { 0, y };
            return { 0, NodeRenderer::NODE_WIDTH };
        }

        float total_width = 0.0f;
        float max_x = 0.0f;
        std::vector<LayoutInfo> child_layouts;

        // Calculate the children layout
        for (uint32_t child_id : node.children_ids)
        {
            auto child_layout = calculatePositions(child_id,
                y + NodeRenderer::NODE_HEIGHT + NodeRenderer::VERTICAL_SPACING);
            child_layouts.push_back(child_layout);
            total_width += child_layout.width;
            max_x = std::max(max_x, child_layout.x + child_layout.width);
        }

        // Add the horizontal spacing between the children
        total_width += NodeRenderer::HORIZONTAL_SPACING * float(node.children_ids.size() - size_t(1));

        // Center the parent node above the children
        float start_x = -total_width / 2.0f;
        node.position = { start_x + total_width / 2.0f, y };

        // Position the children
        float current_x = start_x;
        for (size_t i = 0; i < node.children_ids.size(); ++i)
        {
            auto& child_node = m_nodes[node.children_ids[i]];
            child_node.position.x += current_x + child_layouts[i].width / 2.0f;
            current_x += child_layouts[i].width + NodeRenderer::HORIZONTAL_SPACING;
        }

        return { start_x + total_width / 2.0f, total_width };
    };

    // Calculate the layout from the root
    calculatePositions(root->first, 0);

    // After layout is calculated, adjust camera to show the entire tree
    if (m_tree_received && !m_nodes.empty())
    {
        // Find bounds of the tree
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();

        for (const auto& [id, node] : m_nodes)
        {
            minX = std::min(minX, node.position.x - NodeRenderer::NODE_WIDTH/2.0f);
            maxX = std::max(maxX, node.position.x + NodeRenderer::NODE_WIDTH/2.0f);
            minY = std::min(minY, node.position.y - NodeRenderer::NODE_HEIGHT/2.0f);
            maxY = std::max(maxY, node.position.y + NodeRenderer::NODE_HEIGHT/2.0f);
        }

        // Add padding
        float padding = 50.0f;
        sf::Vector2f center((minX + maxX) / 2.0f, (minY + maxY) / 2.0f);
        sf::Vector2f size(maxX - minX + padding * 2, maxY - minY + padding * 2);

        // Update camera
        m_camera.setCenter(center);
        m_camera.setSize(size);
        m_window.setView(m_camera);
    }
}

} // namespace bt