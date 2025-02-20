#pragma once

#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include "DebugServer.hpp"
#include "NodeRenderer.hpp"
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>
#include <string>

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
    //! \brief Last mouse position
    sf::Vector2f m_last_mouse_pos;
    //! \brief Whether the camera is panning
    bool m_is_panning = false;
};

} // namespace bt