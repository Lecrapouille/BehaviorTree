#pragma once

#include "BehaviorTree/BehaviorTree.hpp"
#include "BehaviorTree/TreeExporter.hpp"

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
#include <system_error>

namespace bt {

// ****************************************************************************
//! \brief Enum for visualizer-specific error codes
// ****************************************************************************
enum class visualizer_errc
{
    tree_has_no_root = 1,
    tree_structure_send_failed = 2,
    invalid_ip_address = 3,
    connection_failed = 4,
    socket_creation_failed = 5,
    connection_timeout = 6,
    serialization_failed = 7,
    send_failed = 8,
    empty_update = 9,
    invalid_socket = 10
};

// ****************************************************************************
//! \brief Error category for BehaviorTreeVisualizer errors
// ****************************************************************************
class visualizer_error_category : public std::error_category
{
public:
    //! \brief Get the name of the error category
    const char* name() const noexcept override { return "behavior_tree_visualizer"; }

    //! \brief Get the error message for a given error code
    std::string message(int ev) const override
    {
        switch (static_cast<visualizer_errc>(ev))
        {
            case visualizer_errc::tree_has_no_root:
                return "Tree has no root";
            case visualizer_errc::tree_structure_send_failed:
                return "Failed to send tree structure";
            case visualizer_errc::invalid_ip_address:
                return "Invalid IP address";
            case visualizer_errc::connection_failed:
                return "Connection failed";
            case visualizer_errc::socket_creation_failed:
                return "Socket creation failed";
            case visualizer_errc::connection_timeout:
                return "Connection timeout";
            case visualizer_errc::serialization_failed:
                return "Failed to serialize data";
            case visualizer_errc::send_failed:
                return "Failed to send data over socket";
            case visualizer_errc::empty_update:
                return "Status update is empty";
            case visualizer_errc::invalid_socket:
                return "Invalid socket or not connected";
            default:
                return "Unknown visualizer error";
        }
    }
};

//! \brief Get the singleton instance of the visualizer error category
inline const std::error_category& visualizer_category() noexcept
{
    static visualizer_error_category instance;
    return instance;
}

//! \brief Make error code from visualizer error
inline std::error_code make_error_code(visualizer_errc e) noexcept
{
    return {static_cast<int>(e), visualizer_category()};
}

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

public:
    // ------------------------------------------------------------------------
    //! \brief Constructor initializing the visualizer.
    //! \param[in] p_tree Reference to the behavior tree to visualize.
    //!
    //! Initializes the internal state but does not start the communication.
    //! Use connect() to establish the connection with the visualizer.
    // ------------------------------------------------------------------------
    explicit BehaviorTreeVisualizer(Tree const& p_tree);

    // ------------------------------------------------------------------------
    //! \brief Destructor.
    //!
    //! Properly stops the communication thread and closes the socket
    //! connection with the client.
    // ------------------------------------------------------------------------
    ~BehaviorTreeVisualizer();

    // ------------------------------------------------------------------------
    //! \brief Connects to the visualizer and starts the communication thread.
    //! \param[in] p_ip IP address to connect to.
    //! \param[in] p_port Port to connect to.
    //! \param[in] p_timeout Connection timeout in milliseconds.
    //! \return std::error_code indicating success or the specific error that occurred.
    // ------------------------------------------------------------------------
    std::error_code connect(const std::string& p_ip, uint16_t p_port, std::chrono::milliseconds p_timeout);

    // ------------------------------------------------------------------------
    //! \brief Updates debug information to be sent to the visualizer.
    //! \return true if the connection is established and the tree structure has
    //! been sent, false otherwise.
    //!
    //! This method captures the current state of all tree nodes and
    //! places them in a queue for sending to the client. The actual sending
    //! is performed asynchronously by the communication thread.
    // ------------------------------------------------------------------------
    bool tick();

    // ------------------------------------------------------------------------
    //! \brief Disconnects from the visualizer and stops the communication thread.
    // ------------------------------------------------------------------------
    void disconnect();

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
    //! \brief Sends the behavior tree structure to the client.
    //! \return std::error_code indicating success or the specific error that occurred.
    //!
    //! Converts the tree structure to YAML using TreeExporter and
    //! sends it to the connected client.
    // ------------------------------------------------------------------------
    std::error_code sendTreeStructure();

    // ------------------------------------------------------------------------
    //! \brief Assigns unique IDs to tree nodes using an infix traversal.
    //! \param[in] p_node Current node being processed.
    //! \param[in,out] p_next_id Next available ID.
    //!
    //! Traverses the tree in a defined order (preorder) and assigns
    //! a unique ID to each node. These IDs are used for state updates.
    // ------------------------------------------------------------------------
    void assignNodeIds(bt::Node const* p_node, uint32_t& p_next_id);

    // ------------------------------------------------------------------------
    //! \brief Captures states of all nodes in the tree using an infix traversal.
    //! \param[in] p_node Current node being processed.
    //! \param[out] p_update Status update structure to fill.
    //!
    //! Traverses the tree and captures the current state of each node.
    // ------------------------------------------------------------------------
    void captureNodeStates(bt::Node const* p_node, StatusUpdate& p_update);

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
    //! \return std::error_code indicating success or the specific error that occurred.
    //!
    //! Formats and sends a message containing state updates.
    // ------------------------------------------------------------------------
    std::error_code sendStatusUpdate(const StatusUpdate& p_update);

private:
    //! \brief Reference to the behavior tree
    bt::Tree const& m_behavior_tree;

    //! \brief Connection status
    bool m_connected = false;

    //! \brief Thread execution flag
    bool m_running = false;

    //! \brief Tree structure transmission flag
    bool m_tree_structure_sent = false;

    //! \brief Map associating nodes with their IDs
    std::unordered_map<bt::Node const*, uint32_t> m_ids;

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
};

} // namespace bt