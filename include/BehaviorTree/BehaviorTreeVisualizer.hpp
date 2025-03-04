#pragma once

#include "BehaviorTree.hpp"
#include "TreeExporter.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace bt {

// Forward declarations
class BehaviorNode;
class BehaviorTree;

// ****************************************************************************
//! \brief Class responsible for communication with the visualizer for
//! graphical representation of the behavior tree.
//!
//! This class establishes a socket connection with an external visualization
//! client. It transmits two types of information:
//! - Tree structure (node hierarchy) in YAML format
//! - Real-time node state updates as ID-status pairs
//!
//! Communication is handled through a dedicated thread to avoid blocking
//! the behavior tree execution.
// ****************************************************************************
class BehaviorTreeVisualizer
{
public:

    // ------------------------------------------------------------------------
    //! \brief Message types for communication protocol
    //!
    //! Defines the different types of messages that can be exchanged
    //! between the visualizer and client.
    // ------------------------------------------------------------------------
    enum class MessageType : uint8_t
    {
        TREE_STRUCTURE = 1,  ///< Message containing tree structure in YAML
        STATE_UPDATE = 2     ///< Message containing state update
    };

    // ------------------------------------------------------------------------
    //! \brief Structure for node state updates
    //!
    //! Groups a set of node states.
    // ------------------------------------------------------------------------
    struct StatusUpdate
    {
        std::vector<std::pair<uint32_t, bt::Status>> states; ///< Node states (ID, status)
    };

    // ------------------------------------------------------------------------
    //! \brief Type de map utilisée pour associer les nœuds à leurs IDs.
    // ------------------------------------------------------------------------
    using NodeToIdMap = std::unordered_map<bt::Node*, uint32_t>;

public:
    // ------------------------------------------------------------------------
    //! \brief Constructor initializing the visualizer.
    //! \param[in] p_bt Reference to the behavior tree to visualize.
    //! \param[in] p_ip IP address to connect to.
    //! \param[in] p_port Port to connect to.
    //!
    //! Initializes the socket connection and starts the communication thread.
    //! The thread waits until a client connects.
    // ------------------------------------------------------------------------
    explicit BehaviorTreeVisualizer(bt::Tree& p_bt, std::string p_ip, uint16_t p_port);

    // ------------------------------------------------------------------------
    //! \brief Destructor.
    //!
    //! Properly stops the communication thread and closes the socket
    //! connection with the client.
    // ------------------------------------------------------------------------
    ~BehaviorTreeVisualizer();

    // ------------------------------------------------------------------------
    //! \brief Updates debug information to be sent to the visualizer.
    //!
    //! This method captures the current state of all tree nodes and
    //! places them in a queue for sending to the client. The actual sending
    //! is performed asynchronously by the communication thread.
    // ------------------------------------------------------------------------
    void updateDebugInfo();

    // ------------------------------------------------------------------------
    //! \brief Checks if a client is currently connected.
    //! \return true if a client is connected, false otherwise.
    //!
    //! Used to determine if debug information will actually be
    //! transmitted to a client.
    // ------------------------------------------------------------------------
    bool isConnected() const { return m_connected; }

    // ------------------------------------------------------------------------
    //! \brief Récupère l'ID associé à un nœud.
    //! \param[in] p_node Pointeur vers le nœud.
    //! \return ID du nœud ou 0 si le nœud n'a pas d'ID.
    //!
    //! Cette méthode permet d'obtenir l'ID attribué à un nœud lors du parcours de l'arbre.
    // ------------------------------------------------------------------------
    uint32_t getNodeId(const bt::Node* p_node) const 
    {
        auto it = m_node_to_id.find(const_cast<bt::Node*>(p_node));
        return (it != m_node_to_id.end()) ? it->second : 0;
    }

    // ------------------------------------------------------------------------
    //! \brief Récupère la map complète des IDs de nœuds.
    //! \return Référence constante vers la map des IDs.
    //!
    //! Cette méthode permet d'accéder à la map associant les nœuds à leurs IDs.
    // ------------------------------------------------------------------------
    const NodeToIdMap& getNodeIdMap() const { return m_node_to_id; }

private:
    // ------------------------------------------------------------------------
    //! \brief Attempts to connect to the visualizer client.
    //!
    //! Creates and configures the socket connection with the client.
    //! If successful, sends the tree structure.
    // ------------------------------------------------------------------------
    void connect();

    // ------------------------------------------------------------------------
    //! \brief Sends the behavior tree structure to the client.
    //!
    //! Converts the tree structure to YAML using TreeExporter and
    //! sends it to the connected client.
    // ------------------------------------------------------------------------
    void sendTreeStructure();

    // ------------------------------------------------------------------------
    //! \brief Assigns unique IDs to tree nodes.
    //! \param[in] p_node Current node being processed.
    //! \param[in,out] p_next_id Next available ID.
    //!
    //! Traverses the tree in a defined order (preorder) and assigns
    //! a unique ID to each node. These IDs are used for state updates.
    // ------------------------------------------------------------------------
    void assignNodeIds(bt::Node::Ptr p_node, uint32_t& p_next_id);

    // ------------------------------------------------------------------------
    //! \brief Captures states of all nodes in the tree.
    //! \param[in] p_node Current node being processed.
    //! \param[out] p_update Status update structure to fill.
    //!
    //! Traverses the tree and captures the current state of each node.
    // ------------------------------------------------------------------------
    void captureNodeStates(bt::Node::Ptr p_node, StatusUpdate& p_update);

    // ------------------------------------------------------------------------
    //! \brief Worker thread function for asynchronous communication.
    //!
    //! This function runs in a separate thread and handles reconnection
    //! attempts and transmission of state updates from the queue.
    // ------------------------------------------------------------------------
    void workerThread();

    // ------------------------------------------------------------------------
    //! \brief Sends a node state update to the client.
    //! \param[in] p_update Status update to send.
    //!
    //! Formats and sends a message containing state updates.
    // ------------------------------------------------------------------------
    void sendStatusUpdate(const StatusUpdate& p_update);

private:
    //! \brief Reference to the behavior tree
    bt::Tree& m_behavior_tree;

    //! \brief Connection status
    bool m_connected = false;

    //! \brief Thread execution flag
    bool m_running = true;

    //! \brief Tree structure transmission flag
    bool m_tree_structure_sent = false;

    //! \brief Map associating nodes with their IDs
    NodeToIdMap m_node_to_id;

    //! \brief Network socket
    int m_socket = -1;

    //! \brief Server IP address
    std::string m_ip;

    //! \brief Server port
    uint16_t m_port;

    //! \brief Communication thread
    std::thread m_worker_thread;

    //! \brief Queue of state updates to send
    std::queue<StatusUpdate> m_status_queue;

    //! \brief Mutex protecting the status update queue
    std::mutex m_queue_mutex;

    //! \brief Buffer for message data
    std::vector<uint8_t> m_buffer;
};

} // namespace bt