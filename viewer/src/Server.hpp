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

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace bt {

// ****************************************************************************
//! \brief Server to receive behavior tree data from the application
//! executing the behavior tree.
//!
//! This server uses TCP sockets to establish a connection with a client
//! and receive messages containing data on the structure and state of the tree.
// ****************************************************************************
class Server
{
public:

    //! \brief Type of callback for receiving messages
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] p_port Server listening port.
    //! \param[in] p_callback Callback called when a message is received.
    // ------------------------------------------------------------------------
    Server(uint16_t p_port, MessageCallback p_callback);

    // ------------------------------------------------------------------------
    //! \brief Destructor. Stops the server automatically if it is running.
    // ------------------------------------------------------------------------
    ~Server();

    // ------------------------------------------------------------------------
    //! \brief Start the server and begin listening for incoming connections.
    //! \return true if the server started successfully, false otherwise.
    // ------------------------------------------------------------------------
    bool start();

    // ------------------------------------------------------------------------
    //! \brief Stop the server and close all connections.
    // ------------------------------------------------------------------------
    void stop();

    // ------------------------------------------------------------------------
    //! \brief Check if the server is connected to a client.
    //! \return true if connected, false otherwise.
    // ------------------------------------------------------------------------
    bool isConnected() const;

    // ------------------------------------------------------------------------
    //! \brief Get the server listening port.
    //! \return The server listening port.
    // ------------------------------------------------------------------------
    inline uint16_t getPort() const
    {
        return m_port;
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Accept incoming connections and handle communication with the
    //! client. This method is executed in a separate thread.
    // ------------------------------------------------------------------------
    void acceptConnection();

    // ------------------------------------------------------------------------
    //! \brief Read a message from the client socket.
    //! Uses poll to wait for data with a timeout.
    // ------------------------------------------------------------------------
    void readMessage();

    // ------------------------------------------------------------------------
    //! \brief Process a received message and call the callback.
    //! \param[in] bytes Number of bytes received in the buffer.
    // ------------------------------------------------------------------------
    void handleMessage(size_t bytes);

    // ------------------------------------------------------------------------
    //! \brief Create a pair of sockets for stopping the server.
    //! These sockets are used to stop the server properly.
    // ------------------------------------------------------------------------
    void createSignalSocket();

private:

    //! \brief Maximum message size in bytes
    static constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB

    //! \brief Buffer for receiving incoming messages
    std::vector<uint8_t> m_receive_buffer;

    //! \brief Callback called when a message is received
    MessageCallback m_message_callback;

    //! \brief Indicate if the server is running
    std::atomic<bool> m_running{false};

    //! \brief Indicate if the server is connected to a client
    std::atomic<bool> m_connected{false};

    //! \brief Server listening port
    uint16_t m_port;

    //! \brief Server socket descriptor
    int m_server_socket = -1;

    //! \brief Client socket descriptor
    int m_client_socket = -1;

    //! \brief Server execution thread
    std::thread m_server_thread;

    //! \brief Signal sockets descriptors for stopping the server
    int m_signal_send_fd = -1;
    int m_signal_recv_fd = -1;

    //! \brief Mutex for protecting access to the client socket
    std::mutex m_socket_mutex;

    //! \brief Buffer for received data
    std::vector<uint8_t> m_buffer;
};

} // namespace bt