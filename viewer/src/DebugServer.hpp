#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <thread>

namespace bt {

// ****************************************************************************
//! \brief Server to receive the behavior tree data from the application running
//! the behavior tree.
// ****************************************************************************
class DebugServer
{
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] port Listening port.
    //! \param[in] onMessageReceived Callback called when a message is received.
    // ------------------------------------------------------------------------
    DebugServer(uint16_t p_port, MessageCallback p_on_message_received);
    
    ~DebugServer();

    // ------------------------------------------------------------------------
    //! \brief Start the server.
    //! \return true if the server is started.
    // ------------------------------------------------------------------------
    bool start();

    // ------------------------------------------------------------------------
    //! \brief Stop the server.
    // ------------------------------------------------------------------------
    void stop();

    // ------------------------------------------------------------------------
    //! \brief Check if the server is connected to a client.
    //! \return true if connected.
    // ------------------------------------------------------------------------
    inline bool isConnected() const { return m_connected; }

private:

    void acceptConnection();
    void readMessage();
    void handleMessage(size_t bytes);
    void createSignalSocket();

private:

    //! \brief Maximum message size
    static constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB

    //! \brief Receive buffer
    std::vector<uint8_t> m_receive_buffer;
    //! \brief Callback called when a message is received
    MessageCallback m_message_callback;
    //! \brief True if the server is running
    bool m_running = false;
    //! \brief True if the server is connected to a client
    bool m_connected = false;
    //! \brief Listening port
    uint16_t m_port;
    //! \brief Server socket
    int m_server_socket = -1;
    //! \brief Client socket
    int m_client_socket = -1;
    //! \brief Server thread
    std::thread m_server_thread;
    //! \brief Signal sockets pour l'arrêt
    int m_signal_send_fd = -1;
    int m_signal_recv_fd = -1;
};

} // namespace bt 