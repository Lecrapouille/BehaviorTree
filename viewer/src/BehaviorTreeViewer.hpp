#pragma once

#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include "DebugServer.hpp"
#include "NodeRenderer.hpp"
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>
#include <string>

namespace YAML { class Node; }

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

private:
    void handleMessage(const std::vector<uint8_t>& data);
    void updateLayout();
    void draw();
    
    // ------------------------------------------------------------------------
    //! \brief Process a YAML node and build the tree structure
    //! \param[in] node The YAML node to process
    //! \param[in] parent_id The ID of the parent node
    // ------------------------------------------------------------------------
    void processYAMLNode(const YAML::Node& node, uint32_t parent_id);

    // ------------------------------------------------------------------------
    //! \brief Find a node at the given screen position
    //! \param[in] screen_pos The screen position to check
    //! \return The ID of the node at the position, or -1 if none
    // ------------------------------------------------------------------------
    uint32_t findNodeAtPosition(const sf::Vector2f& screen_pos);

    // ------------------------------------------------------------------------
    //! \brief Center the view on a specific node
    //! \param[in] node_id The ID of the node to center on
    // ------------------------------------------------------------------------
    void centerOnNode(uint32_t node_id);

private:

    // ------------------------------------------------------------------------
    //! \brief Information about a node
    // ------------------------------------------------------------------------
    struct NodeInfo
    {
        std::string name;                   ///< Name of the node
        bt::Status status;                  ///< Status of the node
        uint32_t parent_id;                 ///< ID of the parent node
        std::vector<uint32_t> children_ids; ///< IDs of the children nodes
        sf::Vector2f position;              ///< Position of the node
    };

    //! \brief Server for communication with the behavior tree
    std::unique_ptr<DebugServer> m_server;
    //! \brief Renderer for the behavior tree
    std::unique_ptr<NodeRenderer> m_renderer;
    //! \brief Map of nodes with their information
    std::unordered_map<uint32_t, NodeInfo> m_nodes;
    //! \brief Whether the tree has been received
    bool m_tree_received = false;
    //! \brief Width of the window
    uint32_t m_width = 1280;
    //! \brief Height of the window
    uint32_t m_height = 720;
    //! \brief Port of the debug server
    uint16_t m_port = 9090;
    //! \brief Window for the behavior tree
    sf::RenderWindow m_window;
    //! \brief Camera for the behavior tree
    sf::View m_camera;
    //! \brief Font for the behavior tree
    sf::Font m_font;
    //! \brief Icon texture for the nodes
    sf::Texture m_icon;
    //! \brief Last mouse position
    sf::Vector2f m_last_mouse_pos;
    //! \brief Whether the camera is panning
    bool m_is_panning = false;
    //! \brief Currently selected node ID
    uint32_t m_selected_node_id = static_cast<uint32_t>(-1);
    //! \brief Zoom level
    float m_zoom_level = 1.0f;
    //! \brief Minimum zoom level
    const float m_min_zoom = 0.1f;
    //! \brief Maximum zoom level
    const float m_max_zoom = 5.0f;
    //! \brief Zoom speed factor
    const float m_zoom_speed = 0.1f;
    //! \brief Whether to show node selection highlight
    bool m_show_selection = true;
};

} // namespace bt