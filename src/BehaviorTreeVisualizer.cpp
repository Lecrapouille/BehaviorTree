//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2025 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
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
#include "BehaviorTree/private/Serialization.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

#include <iostream>

namespace bt {

// ----------------------------------------------------------------------------
BehaviorTreeVisualizer::BehaviorTreeVisualizer(bt::Tree const& p_bt)
    : m_behavior_tree(p_bt)
{
}

// ----------------------------------------------------------------------------
BehaviorTreeVisualizer::~BehaviorTreeVisualizer()
{
    disconnect();
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::disconnect()
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
std::error_code
BehaviorTreeVisualizer::connect(const std::string& p_ip,
                                uint16_t p_port,
                                std::chrono::milliseconds p_timeout)
{
    std::cout << "Connecting to the visualizer (" << p_ip << ":" << p_port
              << " with timeout " << p_timeout.count() << " ms)..."
              << std::endl;

    // Store connection parameters
    m_ip = p_ip;
    m_port = p_port;

    // Check if the tree has a root to avoid segmentation fault
    if (!m_behavior_tree.hasRoot())
    {
        return make_error_code(visualizer_errc::tree_has_no_root);
    }

    // Create a TCP socket
    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0)
    {
        return make_error_code(visualizer_errc::socket_creation_failed);
    }

    // Configure the server address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        close(m_socket);
        m_socket = -1;
        return make_error_code(visualizer_errc::invalid_ip_address);
    }

    // Attempt connection with timeout
    auto start_time = std::chrono::steady_clock::now();
    m_connected = false;

    while (!m_connected)
    {
        // Attempt connection
        if (::connect(m_socket,
                      reinterpret_cast<sockaddr*>(&serverAddr),
                      sizeof(serverAddr)) == 0)
        {
            // Assign unique IDs to nodes using an infix traversal
            uint32_t next_id = 0;
            std::cout << "Assigning node IDs" << std::endl;
            assignNodeIds(&m_behavior_tree.getRoot(), next_id);

            // Send the tree structure
            if (auto ec = sendTreeStructure())
            {
                close(m_socket);
                m_socket = -1;
                m_connected = false;
                return ec;
            }

            // Start the worker thread
            m_running = m_connected = true;
            m_worker_thread =
                std::thread(&BehaviorTreeVisualizer::workerThread, this);
            return std::error_code(); // Success
        }

        // Check if it is a connection error or a real error
        else if ((errno == EINPROGRESS) || (errno == EALREADY) ||
                 (errno == EINTR))
        {
            close(m_socket);
            m_socket = -1;
            return make_error_code(visualizer_errc::connection_failed);
        }
        else
        {
            // Check the timeout
            auto current_time = std::chrono::steady_clock::now();
            if (current_time - start_time > p_timeout)
            {
                close(m_socket);
                m_socket = -1;
                return make_error_code(visualizer_errc::connection_timeout);
            }

            // Small pause to avoid overloading the CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    return std::error_code();
}

// ----------------------------------------------------------------------------
bool BehaviorTreeVisualizer::tick()
{
    // Check if we are connected and if the tree structure has been sent
    if (!m_connected || !m_running || !m_tree_structure_sent)
        return false;

    // Create a status update
    StatusUpdate update;

    // Capture the state of all nodes in the tree
    captureNodeStates(&m_behavior_tree.getRoot(), update);

    // Add the update to the queue for sending by the worker thread
    if (!update.states.empty())
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_status_queue.push(std::move(update));
    }

    return true;
}

// ----------------------------------------------------------------------------
std::error_code BehaviorTreeVisualizer::sendTreeStructure()
{
    if (m_socket < 0)
    {
        return make_error_code(visualizer_errc::invalid_socket);
    }

    try
    {
        Serializer serializer;

        // Message type
        serializer << static_cast<uint8_t>(MessageType::TREE_STRUCTURE);

        // Generate the YAML representation of the tree
        serializer << TreeExporter::toYAML(m_behavior_tree);

        // Send the message
        ssize_t sent_bytes =
            send(m_socket, serializer.data(), serializer.size(), 0);
        if (sent_bytes <= 0)
        {
            m_tree_structure_sent = false;
            return make_error_code(visualizer_errc::send_failed);
        }

        m_tree_structure_sent = true;
        return std::error_code();
    }
    catch (const std::exception&)
    {
        m_tree_structure_sent = false;
        return make_error_code(visualizer_errc::serialization_failed);
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::assignNodeIds(bt::Node const* p_node,
                                           uint32_t& p_next_id)
{
    assert(p_node != nullptr);

    // Assign an ID to the current node
    m_ids[p_node] = p_next_id;
    std::cout << "Node ID: " << p_next_id << " Name: " << p_node->name
              << std::endl;
    p_next_id++;

    // Process the children first if it is a composite node
    if (auto composite = dynamic_cast<Composite const*>(p_node))
    {
        for (auto const& child : composite->getChildren())
        {
            assignNodeIds(child.get(), p_next_id);
        }
    }
    // Process the child if it is a decorator node
    else if (auto decorator = dynamic_cast<Decorator const*>(p_node))
    {
        if (decorator->hasChild())
        {
            assignNodeIds(&(decorator->getChild()), p_next_id);
        }
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::captureNodeStates(bt::Node const* p_node,
                                               StatusUpdate& p_update)
{
    // Get the ID of the node
    auto it = m_ids.find(p_node);
    if (it != m_ids.end())
    {
        // Add the node status to the update
        p_update.states.emplace_back(it->second, p_node->getStatus());
    }

    // Process the children if it is a composite node
    if (auto composite = dynamic_cast<Composite const*>(p_node))
    {
        for (auto const& child : composite->getChildren())
        {
            captureNodeStates(child.get(), p_update);
        }
    }
    // Process the child if it is a decorator node
    else if (auto decorator = dynamic_cast<Decorator const*>(p_node))
    {
        if (decorator->hasChild())
        {
            captureNodeStates(&(decorator->getChild()), p_update);
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
        }

        // Wait a short instant before the next iteration
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
    }

    std::cout << "Stopping the worker thread" << std::endl;
}

// ----------------------------------------------------------------------------
std::error_code
BehaviorTreeVisualizer::sendStatusUpdate(const StatusUpdate& p_update)
{
    // Check if the update is empty
    if (p_update.states.empty())
    {
        return make_error_code(visualizer_errc::empty_update);
    }

    // Check if socket is valid
    if (m_socket < 0)
    {
        return make_error_code(visualizer_errc::invalid_socket);
    }

    try
    {
        Serializer serializer;

        // Message type
        serializer << static_cast<uint8_t>(MessageType::STATE_UPDATE);

        // Number of state updates
        serializer << static_cast<uint32_t>(p_update.states.size());

        // Add each state update (ID, status)
        for (const auto& [id, status] : p_update.states)
        {
            serializer << id << static_cast<uint8_t>(status);
        }

        // Send the message
        if (send(m_socket, serializer.data(), serializer.size(), 0) <= 0)
        {
            return make_error_code(visualizer_errc::send_failed);
        }

        return std::error_code();
    }
    catch (const std::exception&)
    {
        return make_error_code(visualizer_errc::serialization_failed);
    }
}

} // namespace bt