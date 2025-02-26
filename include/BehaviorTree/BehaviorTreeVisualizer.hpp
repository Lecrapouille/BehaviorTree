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
    //! Groups a set of node states at a given time.
    // ------------------------------------------------------------------------
    struct StatusUpdate
    {
        std::chrono::system_clock::time_point timestamp; ///< Update timestamp
        std::vector<std::pair<uint32_t, bt::Status>> states; ///< Node states (ID, status)
    };

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

private:
    // ------------------------------------------------------------------------
    //! \brief Establishes connection with the visualizer.
    //!
    //! Configures and opens the socket in server mode on the defined port.
    //! Waits for a client to connect in a blocking manner.
    // ------------------------------------------------------------------------
    void connect();

    // ------------------------------------------------------------------------
    //! \brief Sends the complete tree structure to the visualizer in YAML format.
    //!
    //! Uses TreeExporter to generate a YAML representation of the tree structure
    //! and sends it to the client. This operation is performed only once when
    //! the client connects.
    // ------------------------------------------------------------------------
    void sendTreeStructure();

    // ------------------------------------------------------------------------
    //! \brief Assigns unique IDs to nodes in the tree using infix traversal.
    //! \param[in] p_node Current node to process
    //! \param[inout] p_next_id Next available ID for nodes
    //!
    //! Recursively traverses the tree assigning unique IDs to each node
    //! using infix traversal order.
    // ------------------------------------------------------------------------
    void assignNodeIds(bt::Node::Ptr p_node, uint32_t& p_next_id);

    // ------------------------------------------------------------------------
    //! \brief Captures the state of all tree nodes.
    //! \param[in] p_node Current node to process
    //! \param[out] p_update Structure receiving node states
    //!
    //! Recursively traverses the tree and collects the execution state
    //! (SUCCESS, FAILURE, RUNNING) of each node.
    // ------------------------------------------------------------------------
    void captureNodeStates(bt::Node::Ptr p_node, StatusUpdate& p_update);

    // ------------------------------------------------------------------------
    //! \brief Main communication thread function.
    //!
    //! Handles client connection and state update transmission.
    //! Runs continuously until object destruction.
    // ------------------------------------------------------------------------
    void workerThread();

    // ------------------------------------------------------------------------
    //! \brief Sends a state update to the visualizer.
    //! \param[in] p_update Structure containing states to send
    //!
    //! Serializes and sends node states to the client via socket.
    // ------------------------------------------------------------------------
    void sendStatusUpdate(const StatusUpdate& p_update);

private:

    //! Behavior tree to visualize
    bt::Tree& m_behavior_tree;
    //! Whether the client is connected
    bool m_connected = false;
    //! Whether the worker thread is running
    bool m_running = true;
    //! Whether the tree structure has been sent
    bool m_tree_structure_sent = false;
    //! Map of behavior node to its ID
    std::unordered_map<bt::Node*, uint32_t> m_node_to_id;
    //! Socket for the debug server
    int m_socket = -1;
    //! IP address
    std::string m_ip;
    //! Port number
    uint16_t m_port;
    //! Worker thread
    std::thread m_worker_thread;
    //! Queue of status updates
    std::queue<StatusUpdate> m_status_queue;
    //! Mutex for the queue
    std::mutex m_queue_mutex;
};

} // namespace bt