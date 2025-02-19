#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <thread>

namespace bt {

/// @brief Serveur de débogage pour recevoir les données de l'arbre de comportement
class DebugServer {
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    /// @brief Constructeur
    /// @param[in] port Port d'écoute du serveur
    /// @param[in] onMessageReceived Callback appelé à la réception d'un message
    explicit DebugServer(
        uint16_t port = 9090,
        MessageCallback onMessageReceived = nullptr
    );
    
    ~DebugServer();

    /// @brief Démarre le serveur
    /// @return true si le démarrage est réussi
    bool start();

    /// @brief Arrête le serveur
    void stop();

    /// @brief Vérifie si le serveur est connecté à un client
    /// @return true si connecté
    bool isConnected() const { return connected; }

private:
    void acceptConnection();
    void readMessage();
    void handleMessage(size_t bytes);

    MessageCallback messageCallback;
    bool running{false};
    bool connected{false};
    uint16_t port;

    int serverSocket{-1};
    int clientSocket{-1};
    
    std::vector<uint8_t> receiveBuffer;
    static constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB

    std::thread serverThread;
};

} // namespace bt 