//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2024 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//*****************************************************************************

#include "Server.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sys/eventfd.h>
#include <poll.h>
#include <iostream>

namespace bt {

// ----------------------------------------------------------------------------
Server::Server(uint16_t p_port, MessageCallback p_on_message_received)
    : m_message_callback(std::move(p_on_message_received)),
      m_port(p_port)
{
    // Allocate the receive buffer with an initial size
    m_receive_buffer.resize(MAX_MESSAGE_SIZE);

    // Create the signal sockets for proper shutdown
    createSignalSocket();
}

// ----------------------------------------------------------------------------
Server::~Server()
{
    stop();
}

// ----------------------------------------------------------------------------
bool Server::start()
{
    // Check if the server is already running
    if (m_running)
    {
        std::cerr << "Server is already running" << std::endl;
        return false;
    }

    // Create the server socket
    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket < 0)
    {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Configure the socket to reuse the address
    int reuse = 1;
    if (setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        std::cerr << "Error setting socket options: " << strerror(errno) << std::endl;
        close(m_server_socket);
        m_server_socket = -1;
        return false;
    }

    // Configure the server address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_port);

    // Bind the socket to the address
    if (bind(m_server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(m_server_socket);
        m_server_socket = -1;
        return false;
    }

    // Listen on the socket
    if (listen(m_server_socket, 1) < 0)
    {
        std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
        close(m_server_socket);
        m_server_socket = -1;
        return false;
    }

    // Start the server thread
    // std::cout << "Debug server started on port " << m_port << std::endl;
    m_running = true;
    m_server_thread = std::thread(&Server::acceptConnection, this);

    return true;
}

// ----------------------------------------------------------------------------
void Server::stop()
{
    // Check if the server is running
    if (!m_running)
    {
        return;
    }

    // Signal the stop to the server thread
    m_running = false;

    // Send a signal to unlock the thread
    if (m_signal_send_fd >= 0)
    {
        char byte = 1;
        send(m_signal_send_fd, &byte, 1, 0);
    }

    // Wait for the thread to finish
    if (m_server_thread.joinable())
    {
        m_server_thread.join();
    }

    // Close the sockets
    if (m_client_socket >= 0)
    {
        close(m_client_socket);
        m_client_socket = -1;
    }

    // Close the server socket
    if (m_server_socket >= 0)
    {
        close(m_server_socket);
        m_server_socket = -1;
    }

    // Close the signal sockets
    if (m_signal_send_fd >= 0)
    {
        close(m_signal_send_fd);
        m_signal_send_fd = -1;
    }

    // Close the signal socket
    if (m_signal_recv_fd >= 0)
    {
        close(m_signal_recv_fd);
        m_signal_recv_fd = -1;
    }

    m_connected = false;
    std::cout << "Debug server stopped" << std::endl;
}

// ----------------------------------------------------------------------------
void Server::acceptConnection()
{
    std::cout << "Waiting for connection on port " << m_port << " ..." << std::endl;

    // Configure the file descriptors for poll
    pollfd fds[2];
    fds[0].fd = m_server_socket;
    fds[0].events = POLLIN;
    fds[1].fd = m_signal_recv_fd;
    fds[1].events = POLLIN;

    while (m_running)
    {
        // Wait for an event on the sockets
        int ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            // If the error is due to an interrupt, continue
            if (errno == EINTR)
                continue;

            std::cerr << "Error waiting for connection: " << strerror(errno) << std::endl;
            break;
        }

        // Check if a stop signal has been received
        if (fds[1].revents & POLLIN)
        {
            char byte;
            recv(m_signal_recv_fd, &byte, 1, 0);
            break;
        }

        // Check if an incoming connection is available
        if (fds[0].revents & POLLIN)
        {
            // Accept the connection
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            m_client_socket = accept(m_server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);

            // If the connection fails, continue
            if (m_client_socket < 0)
            {
                std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
                continue;
            }

            // Display the connection information
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            std::cout << "Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

            // Mark as connected and start reading messages
            m_connected = true;

            // Read messages as long as the client is connected
            while (m_running && m_connected)
            {
                readMessage();
            }

            // Close the client socket
            if (m_client_socket >= 0)
            {
                close(m_client_socket);
                m_client_socket = -1;
            }

            m_connected = false;
            std::cout << "Client disconnected" << std::endl;
        }
    }
}

// ----------------------------------------------------------------------------
void Server::readMessage()
{
    // Configure the file descriptors for poll
    pollfd fds[2];
    fds[0].fd = m_client_socket;
    fds[0].events = POLLIN;
    fds[1].fd = m_signal_recv_fd;
    fds[1].events = POLLIN;

    // Wait for an event on the sockets
    int ret = poll(fds, 2, 1000); // Timeout of 1 second
    if (ret < 0)
    {
        // If the error is due to an interrupt, return
        if (errno == EINTR)
            return;

        std::cerr << "Error waiting for data: " << strerror(errno) << std::endl;
        m_connected = false;
        return;
    }

    // Check if a stop signal has been received
    if (fds[1].revents & POLLIN)
    {
        char byte;
        recv(m_signal_recv_fd, &byte, 1, 0);
        m_connected = false;
        return;
    }

    // Check if data is available
    if (fds[0].revents & POLLIN)
    {
        // Read the data
        ssize_t bytes = recv(m_client_socket, m_receive_buffer.data(), m_receive_buffer.size(), 0);
        if (bytes <= 0)
        {
            // Error or connection closed
            if (bytes < 0)
                std::cerr << "Error reading data: " << strerror(errno) << std::endl;

            m_connected = false;
            return;
        }

        // Process the received message
        handleMessage(static_cast<size_t>(bytes));
    }
}

// ----------------------------------------------------------------------------
void Server::handleMessage(size_t bytes)
{
    // Check if the callback is defined
    if (!m_message_callback)
    {
        std::cerr << "No callback defined for messages" << std::endl;
        return;
    }

    // Create a copy of the received data for the callback
    std::vector<uint8_t> message_data(
        m_receive_buffer.begin(),
        m_receive_buffer.begin() + static_cast<long int>(bytes));

    try
    {
        // Call the callback with the received data
        m_message_callback(message_data);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in message callback: " << e.what() << std::endl;
    }
}

// ----------------------------------------------------------------------------
void Server::createSignalSocket()
{
    // Create a pair of sockets for signaling
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
    {
        std::cerr << "Error creating signal sockets: " << strerror(errno) << std::endl;
        return;
    }

    // Store the file descriptors
    m_signal_send_fd = sockets[0];
    m_signal_recv_fd = sockets[1];

    // Configure the sockets in non-blocking mode
    fcntl(m_signal_send_fd, F_SETFL, O_NONBLOCK);
    fcntl(m_signal_recv_fd, F_SETFL, O_NONBLOCK);
}

// ----------------------------------------------------------------------------
bool Server::isConnected() const
{
    return m_connected.load();
}

} // namespace bt