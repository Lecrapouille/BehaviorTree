#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <thread>

namespace bt {

// ****************************************************************************
//! \brief Serveur pour recevoir les données de l'arbre de comportement depuis l'application
//! exécutant l'arbre de comportement.
//!
//! Ce serveur utilise des sockets TCP pour établir une connexion avec un client
//! et recevoir des messages contenant des données sur la structure et l'état de l'arbre.
// ****************************************************************************
class DebugServer
{
public:
    //! \brief Type de callback pour la réception de messages
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    // ------------------------------------------------------------------------
    //! \brief Constructeur.
    //! \param[in] p_port Port d'écoute du serveur.
    //! \param[in] p_on_message_received Callback appelé lorsqu'un message est reçu.
    // ------------------------------------------------------------------------
    DebugServer(uint16_t p_port, MessageCallback p_on_message_received);

    // ------------------------------------------------------------------------
    //! \brief Destructeur. Arrête automatiquement le serveur s'il est en cours d'exécution.
    // ------------------------------------------------------------------------
    ~DebugServer();

    // ------------------------------------------------------------------------
    //! \brief Démarre le serveur et commence à écouter les connexions entrantes.
    //! \return true si le serveur a démarré avec succès, false sinon.
    // ------------------------------------------------------------------------
    bool start();

    // ------------------------------------------------------------------------
    //! \brief Arrête le serveur et ferme toutes les connexions.
    // ------------------------------------------------------------------------
    void stop();

    // ------------------------------------------------------------------------
    //! \brief Vérifie si le serveur est connecté à un client.
    //! \return true si connecté, false sinon.
    // ------------------------------------------------------------------------
    inline bool isConnected() const { return m_connected; }

private:
    // ------------------------------------------------------------------------
    //! \brief Accepte les connexions entrantes et gère la communication avec le client.
    //! Cette méthode est exécutée dans un thread séparé.
    // ------------------------------------------------------------------------
    void acceptConnection();

    // ------------------------------------------------------------------------
    //! \brief Lit un message depuis le socket client.
    //! Utilise poll pour attendre des données avec un timeout.
    // ------------------------------------------------------------------------
    void readMessage();

    // ------------------------------------------------------------------------
    //! \brief Traite un message reçu et appelle le callback.
    //! \param[in] bytes Nombre d'octets reçus dans le buffer.
    // ------------------------------------------------------------------------
    void handleMessage(size_t bytes);

    // ------------------------------------------------------------------------
    //! \brief Crée une paire de sockets pour la signalisation d'arrêt.
    //! Ces sockets sont utilisés pour interrompre proprement le thread serveur.
    // ------------------------------------------------------------------------
    void createSignalSocket();

private:
    //! \brief Taille maximale d'un message en octets
    static constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB

    //! \brief Buffer de réception pour les messages entrants
    std::vector<uint8_t> m_receive_buffer;

    //! \brief Callback appelé lorsqu'un message est reçu
    MessageCallback m_message_callback;

    //! \brief Indique si le serveur est en cours d'exécution
    bool m_running = false;

    //! \brief Indique si le serveur est connecté à un client
    bool m_connected = false;

    //! \brief Port d'écoute du serveur
    uint16_t m_port;

    //! \brief Descripteur du socket serveur
    int m_server_socket = -1;

    //! \brief Descripteur du socket client
    int m_client_socket = -1;

    //! \brief Thread d'exécution du serveur
    std::thread m_server_thread;

    //! \brief Descripteurs des sockets de signalisation pour l'arrêt propre
    int m_signal_send_fd = -1;
    int m_signal_recv_fd = -1;
};

} // namespace bt