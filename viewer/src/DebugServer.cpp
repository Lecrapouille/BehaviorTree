#include "DebugServer.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sys/eventfd.h>
#include <poll.h>
#include <iostream>

namespace bt {

// ----------------------------------------------------------------------------
DebugServer::DebugServer(uint16_t p_port, MessageCallback p_on_message_received)
    : m_message_callback(std::move(p_on_message_received)), m_port(p_port)
{
    m_receive_buffer.resize(MAX_MESSAGE_SIZE);
}

// ----------------------------------------------------------------------------
DebugServer::~DebugServer()
{
    stop();
}

// ----------------------------------------------------------------------------
bool DebugServer::start()
{
    std::cout << "Starting the debug server" << std::endl;

    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket < 0)
    {
        std::cerr << "Error creating the server socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Create the signal socket
    m_signal_recv_fd = eventfd(0, EFD_NONBLOCK);
    if (m_signal_recv_fd < 0)
    {
        std::cerr << "Error creating the signal socket: " << strerror(errno) << std::endl;
        close(m_server_socket);
        return false;
    }
    m_signal_send_fd = m_signal_recv_fd;

    // Allow the reuse of the address
    int opt = 1;
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configuration of the address
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);

    // Bind the socket to the address
    if (bind(m_server_socket, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0)
    {
        std::cerr << "Error binding the socket to the address: " << strerror(errno) << std::endl;
        close(m_server_socket);
        return false;
    }

    // Listen for incoming connections
    if (listen(m_server_socket, 1) < 0)
    {
        std::cerr << "Error listening for incoming connections: " << strerror(errno) << std::endl;
        close(m_server_socket);
        return false;
    }

    // Start the server thread
    m_running = true;
    m_server_thread = std::thread([this]()
    {
        while (m_running)
        {
            acceptConnection();
        }
        std::cout << "Server thread stopped" << std::endl;
    });

    return true;
}

// ----------------------------------------------------------------------------
void DebugServer::stop()
{
    m_running = false;
    m_connected = false;

    // Send the stop signal
    if (m_signal_send_fd >= 0)
    {
        uint64_t value = 1;
        write(m_signal_send_fd, &value, sizeof(value));
    }

    // Close the client socket
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

    // Join the server thread
    if (m_server_thread.joinable())
    {
        m_server_thread.join();
    }

    // Close the signal socket
    if (m_signal_recv_fd >= 0)
    {
        close(m_signal_recv_fd);
        m_signal_recv_fd = -1;
        m_signal_send_fd = -1;
    }
}

// ----------------------------------------------------------------------------
void DebugServer::acceptConnection()
{
    struct pollfd fds[2];
    fds[0].fd = m_server_socket;
    fds[0].events = POLLIN;
    fds[1].fd = m_signal_recv_fd;
    fds[1].events = POLLIN;

    while (m_running)
    {
        // Wait for events
        int ret = poll(fds, 2, 100); // Timeout of 100ms
        if (ret < 0)
        {
            // Interruption
            if (errno == EINTR)
                continue;
            break;
        }

        // Check the stop signal
        if (fds[1].revents & POLLIN)
        {
            break;
        }

        // Check new connections
        if (fds[0].revents & POLLIN)
        {
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);

            // Accept the new connection
            int new_client = accept(m_server_socket, 
                                  reinterpret_cast<struct sockaddr*>(&client_addr), 
                                  &client_addr_len);
            if (new_client < 0)
            {
                std::cerr << "Error accepting new connection: " << strerror(errno) << std::endl;
                continue;
            }

            std::cout << "New connection accepted" << std::endl;

            // Close the previous connection
            if (m_client_socket >= 0)
            {
                close(m_client_socket);
            }

            // Set the new connection
            m_client_socket = new_client;
            m_connected = true;

            // Read messages
            while (m_running && m_connected)
            {
                readMessage();
            }
        }
    }
}

// ----------------------------------------------------------------------------
void DebugServer::readMessage()
{
    ssize_t bytes_read = recv(m_client_socket, m_receive_buffer.data(), m_receive_buffer.size(), 0);
    
    if (bytes_read <= 0)
    {
        // Error or disconnection
        std::cerr << "Error reading message: " << strerror(errno) << std::endl;
        m_connected = false;
        close(m_client_socket);
        m_client_socket = -1;
        return;
    }

    handleMessage(static_cast<size_t>(bytes_read));
}

// ----------------------------------------------------------------------------
void DebugServer::handleMessage(size_t bytes)
{
    if (m_message_callback)
    {
        m_message_callback(std::vector<uint8_t>(
            m_receive_buffer.begin(), m_receive_buffer.begin() + static_cast<std::ptrdiff_t>(bytes)));
    }
}

} // namespace bt 