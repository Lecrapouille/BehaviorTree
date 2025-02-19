#include "DebugServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

namespace bt {

DebugServer::DebugServer(uint16_t port_, MessageCallback onMessageReceived)
    : messageCallback(std::move(onMessageReceived))
    , port(port_)
{
    receiveBuffer.resize(MAX_MESSAGE_SIZE);
}

DebugServer::~DebugServer()
{
    stop();
}

bool DebugServer::start()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        return false;
    }

    // Permet la réutilisation de l'adresse
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configuration de l'adresse
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(serverSocket);
        return false;
    }

    if (listen(serverSocket, 1) < 0) {
        close(serverSocket);
        return false;
    }

    running = true;
    serverThread = std::thread([this]() {
        while (running) {
            acceptConnection();
        }
    });

    return true;
}

void DebugServer::stop()
{
    running = false;
    connected = false;

    if (clientSocket >= 0) {
        close(clientSocket);
        clientSocket = -1;
    }

    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }

    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void DebugServer::acceptConnection()
{
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);

    int newClient = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (newClient < 0) {
        return;
    }

    // Ferme la connexion précédente si elle existe
    if (clientSocket >= 0) {
        close(clientSocket);
    }

    clientSocket = newClient;
    connected = true;

    // Boucle de lecture des messages
    while (running && connected) {
        readMessage();
    }
}

void DebugServer::readMessage()
{
    ssize_t bytesRead = recv(clientSocket, receiveBuffer.data(), receiveBuffer.size(), 0);
    
    if (bytesRead <= 0) {
        // Erreur ou déconnexion
        connected = false;
        close(clientSocket);
        clientSocket = -1;
        return;
    }

    handleMessage(static_cast<size_t>(bytesRead));
}

void DebugServer::handleMessage(size_t bytes)
{
    if (messageCallback) {
        messageCallback(std::vector<uint8_t>(receiveBuffer.begin(), 
                                           receiveBuffer.begin() + bytes));
    }
}

} // namespace bt 