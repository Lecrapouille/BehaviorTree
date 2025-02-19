#pragma once

#include "BehaviorTree.hpp"
#include "NodeStatus.hpp"
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

class BehaviorNode;
class BehaviorTree;

// ****************************************************************************
//! \brief Class responsible for communication with the visualizer for
//! graphical representation of the behavior tree.
//!
//! This class establishes a socket connection with an external visualization
//! client. It transmits two types of information:
//! - Tree structure (node hierarchy)
//! - Real-time node state updates
//!
//! Communication is handled through a dedicated thread to avoid blocking
//! the behavior tree execution.
// ****************************************************************************
class BehaviorTreeVisualizer
{
public:
    // ------------------------------------------------------------------------
    //! \brief Constructor initializing the visualizer.
    //! \param[in] bt Reference to the behavior tree to visualize.
    //!
    //! Initializes the socket connection and starts the communication thread.
    //! The thread waits until a client connects.
    // ------------------------------------------------------------------------
    explicit BehaviorTreeVisualizer(BehaviorTree& bt);

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
    //! \brief Sends the complete tree structure to the visualizer.
    //!
    //! Traverses the tree to build a representation of its structure
    //! (nodes and their relationships) and sends it to the client. This operation
    //! is performed only once when the client connects.
    // ------------------------------------------------------------------------
    void sendTreeStructure();

    // ------------------------------------------------------------------------
    //! \brief Builds the tree structure recursively.
    //! \param[in] node Current node to process
    //! \param[out] nodes Vector storing the node structure
    //! \param[inout] nextId Next available ID for nodes
    //! \param[in] parentId Parent node ID (-1 for root)
    //!
    //! Recursively traverses the tree assigning unique IDs to each node
    //! and building parent-child relationships.
    // ------------------------------------------------------------------------
    void buildTreeStructure(BehaviorNode* node, 
                          std::vector<NodeStructure>& nodes,
                          uint32_t& nextId, 
                          uint32_t parentId = -1);

    // ------------------------------------------------------------------------
    //! \brief Captures the state of all tree nodes.
    //! \param[in] node Current node to process
    //! \param[out] update Structure receiving node states
    //!
    //! Recursively traverses the tree and collects the execution state
    //! (SUCCESS, FAILURE, RUNNING) of each node.
    // ------------------------------------------------------------------------
    void captureNodeStates(BehaviorNode* node, StatusUpdate& update);

    // ------------------------------------------------------------------------
    //! \brief Main communication thread function.
    //!
    //! Handles client connection and state update transmission.
    //! Runs continuously until object destruction.
    // ------------------------------------------------------------------------
    void workerThread();

    // ------------------------------------------------------------------------
    //! \brief Sends a state update to the visualizer.
    //! \param[in] update Structure containing states to send
    //!
    //! Serializes and sends node states to the client via socket.
    // ------------------------------------------------------------------------
    void sendStatusUpdate(const StatusUpdate& update);

private:
    // ------------------------------------------------------------------------
    //! \brief Message types for communication protocol
    //!
    //! Defines the different types of messages that can be exchanged
    //! between the visualizer and client.
    // ------------------------------------------------------------------------
    enum class MessageType : uint8_t {
        treeStructure = 1,  ///< Message containing tree structure
        stateUpdate = 2     ///< Message containing state update
    };

    // ------------------------------------------------------------------------
    //! \brief Structure describing a node in the tree
    //!
    //! Contains all necessary information to represent
    //! a node and its relationships in the tree.
    // ------------------------------------------------------------------------
    struct NodeStructure {
        uint32_t id;                       ///< Unique node identifier
        std::string name;                  ///< Node name/type
        uint32_t parentId;                 ///< Parent ID (-1 for root)
        std::vector<uint32_t> childrenIds; ///< List of child node IDs
    };

    // ------------------------------------------------------------------------
    //! \brief Structure for node state updates
    //!
    //! Groups a set of node states at a given time.
    // ------------------------------------------------------------------------
    struct StatusUpdate {
        std::chrono::system_clock::time_point timestamp; ///< Update timestamp
        std::vector<std::pair<uint32_t, bt::NodeStatus>> states; ///< Node states (ID, status)
    };

    //! Behavior tree to visualize
    BehaviorTree& m_behaviorTree;
    //! Whether the client is connected
    bool m_connected{false};
    //! Whether the worker thread is running
    bool m_running{true};
    //! Whether the tree structure has been sent
    bool m_treeStructureSent{false};
    //! Map of behavior node to its ID
    std::unordered_map<BehaviorNode*, uint32_t> m_nodeToId;
    //! Socket for the debug server
    int m_socket{-1};
    //! Worker thread
    std::thread m_workerThread;
    //! Queue of status updates
    std::queue<StatusUpdate> m_statusQueue;
    //! Mutex for the queue
    std::mutex m_queueMutex;
}; 