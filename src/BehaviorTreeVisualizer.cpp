#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include "BehaviorTree/TreeExporter.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <sstream>

namespace bt
{

// ----------------------------------------------------------------------------
BehaviorTreeVisualizer::BehaviorTreeVisualizer(bt::Tree& p_bt, std::string p_ip, uint16_t p_port)
    : m_behavior_tree(p_bt), m_ip(p_ip), m_port(p_port)
{
    // Start the communication thread in the background
    m_worker_thread = std::thread(&BehaviorTreeVisualizer::workerThread, this);
}

// ----------------------------------------------------------------------------
BehaviorTreeVisualizer::~BehaviorTreeVisualizer()
{
    // Signal the thread to stop
    m_running = false;

    // Close the socket if it is open
    if (m_socket >= 0)
    {
        close(m_socket);
        m_socket = -1;
    }

    // Wait for the thread to terminate properly
    if (m_worker_thread.joinable())
    {
        m_worker_thread.join();
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::updateDebugInfo()
{
    // Check if we are connected and if the tree structure has been sent
    if (!m_connected || !m_tree_structure_sent)
    {
        std::cout << "Not connected or tree structure not sent, not updating states" << std::endl;
        return;
    }

    // Create a status update
    StatusUpdate update;

    // Capture the state of all nodes in the tree
    captureNodeStates(m_behavior_tree.getRoot(), update);

    // If no state has been captured, do not send an update
    if (update.states.empty())
    {
        return;
    }

    // Add the update to the queue for sending by the worker thread
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_status_queue.push(std::move(update));
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::connect()
{
    std::cout << "Tentative de connexion à " << m_ip << ":" << m_port << std::endl;

    // Create a TCP socket
    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0)
    {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        m_connected = false;
        return;
    }

    // Configure the server address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid IP address: " << m_ip << std::endl;
        close(m_socket);
        m_socket = -1;
        m_connected = false;
        return;
    }

    // Connect to the server
    std::cout << "Attempting to connect to the server..." << std::endl;
    if (::connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
    {
        std::cerr << "Connection failed: " << strerror(errno) << std::endl;
        close(m_socket);
        m_socket = -1;
        m_connected = false;
        return;
    }

    std::cout << "Connection established successfully!" << std::endl;
    m_connected = true;

    // Assign unique IDs to nodes using an infix traversal
    uint32_t next_id = 0;
    assignNodeIds(m_behavior_tree.getRoot(), next_id);

    // Send the tree structure
    sendTreeStructure();
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::sendTreeStructure()
{
    std::cout << "Sending the tree structure in YAML..." << std::endl;

    try {
        // Generate the YAML representation of the tree
        std::string yaml_tree = TreeExporter::toYAML(m_behavior_tree, &m_node_to_id);

        // Clear the buffer
        m_buffer.clear();
        m_buffer.reserve(yaml_tree.length() + sizeof(uint32_t) + sizeof(uint8_t));

        // Message type
        m_buffer.push_back(static_cast<uint8_t>(MessageType::TREE_STRUCTURE));

        // YAML string length
        uint32_t yaml_length = static_cast<uint32_t>(yaml_tree.length());
        m_buffer.insert(m_buffer.end(),
                     reinterpret_cast<const uint8_t*>(&yaml_length),
                     reinterpret_cast<const uint8_t*>(&yaml_length) + sizeof(yaml_length));

        // YAML string content
        m_buffer.insert(m_buffer.end(), yaml_tree.begin(), yaml_tree.end());

        // Send the message
        ssize_t sent_bytes = send(m_socket, m_buffer.data(), m_buffer.size(), 0);
        if (sent_bytes <= 0)
        {
            std::cerr << "Error sending the YAML structure: " << strerror(errno) << std::endl;
            m_connected = false;
            close(m_socket);
            m_socket = -1;
        }
        else
        {
            std::cout << "Tree structure sent in YAML (" << sent_bytes << " bytes)" << std::endl;
            m_tree_structure_sent = true;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception generating or sending the YAML: " << e.what() << std::endl;
        m_connected = false;
        close(m_socket);
        m_socket = -1;
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::assignNodeIds(bt::Node::Ptr p_node, uint32_t& p_next_id)
{
    if (!p_node)
        return;

    // Infix traversal: first the children, then the current node

    // Process the children first if it is a composite node
    if (auto composite = std::dynamic_pointer_cast<Composite>(p_node))
    {
        for (auto const& child : composite->getChildren())
        {
            assignNodeIds(child, p_next_id);
        }
    }
    // Process the child if it is a decorator node
    else if (auto decorator = std::dynamic_pointer_cast<Decorator>(p_node))
    {
        if (decorator->getChild())
        {
            assignNodeIds(decorator->getChild(), p_next_id);
        }
    }

    // Assign an ID to the current node
    m_node_to_id[p_node.get()] = p_next_id++;

    std::cout << "Node '" << p_node->name << "' assigned to ID " << m_node_to_id[p_node.get()] << std::endl;
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::captureNodeStates(bt::Node::Ptr p_node, StatusUpdate& p_update)
{
    if (!p_node)
        return;

    // Get the ID of the node
    auto it = m_node_to_id.find(p_node.get());
    if (it != m_node_to_id.end())
    {
        // Add the node status to the update
        p_update.states.emplace_back(it->second, p_node->getStatus());
    }

    // Recursively process the children

    // Process the children if it is a composite node
    if (auto composite = std::dynamic_pointer_cast<Composite>(p_node))
    {
        for (auto const& child : composite->getChildren())
        {
            captureNodeStates(child, p_update);
        }
    }
    // Process the child if it is a decorator node
    else if (auto decorator = std::dynamic_pointer_cast<Decorator>(p_node))
    {
        if (decorator->getChild())
        {
            captureNodeStates(decorator->getChild(), p_update);
        }
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::workerThread()
{
    std::cout << "Starting the worker thread" << std::endl;

    // Main loop of the thread
    while (m_running)
    {
        // If we are not connected, try to connect
        if (!m_connected)
        {
            std::cout << "Not connected, trying to reconnect..." << std::endl;
            connect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Get all the pending updates
        std::vector<StatusUpdate> updates;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            while (!m_status_queue.empty())
            {
                updates.push_back(std::move(m_status_queue.front()));
                m_status_queue.pop();
            }
        }

        // Send each update
        for (const auto& update : updates)
        {
            sendStatusUpdate(update);

            // If the sending failed and we are disconnected, exit the loop
            if (!m_connected)
                break;
        }

        // Wait a short instant before the next iteration
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
    }

    std::cout << "Stopping the worker thread" << std::endl;
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::sendStatusUpdate(const StatusUpdate& p_update)
{
    // Do nothing if the update is empty
    if (p_update.states.empty())
        return;

    std::cout << "Sending the state update for " << p_update.states.size() << " nodes" << std::endl;

    try {
        // Clear the buffer
        m_buffer.clear();

        // Message type
        m_buffer.push_back(static_cast<uint8_t>(MessageType::STATE_UPDATE));

        // Number of state updates
        uint32_t count = static_cast<uint32_t>(p_update.states.size());
        m_buffer.insert(m_buffer.end(),
                     reinterpret_cast<uint8_t*>(&count),
                     reinterpret_cast<uint8_t*>(&count) + sizeof(count));

        // Add each state update (ID, status)
        for (const auto& [id, status] : p_update.states)
        {
            // Node ID
            m_buffer.insert(m_buffer.end(),
                         reinterpret_cast<const uint8_t*>(&id),
                         reinterpret_cast<const uint8_t*>(&id) + sizeof(id));

            // Node status
            uint8_t status_value = static_cast<uint8_t>(status);
            m_buffer.push_back(status_value);
        }

        // Send the message
        if (send(m_socket, m_buffer.data(), m_buffer.size(), 0) <= 0)
        {
            std::cerr << "Error sending the state update: " << strerror(errno) << std::endl;
            m_connected = false;
            close(m_socket);
            m_socket = -1;
        }
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Exception sending the state update: " << e.what() << std::endl;
        m_connected = false;
        close(m_socket);
        m_socket = -1;
    }
}

} // namespace bt