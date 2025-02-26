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
    // Démarrer le thread de communication en arrière-plan
    m_worker_thread = std::thread(&BehaviorTreeVisualizer::workerThread, this);
}

// ----------------------------------------------------------------------------
BehaviorTreeVisualizer::~BehaviorTreeVisualizer()
{
    // Signaler au thread de s'arrêter
    m_running = false;

    // Fermer la connexion socket si elle est ouverte
    if (m_socket >= 0)
    {
        close(m_socket);
        m_socket = -1;
    }

    // Attendre que le thread se termine proprement
    if (m_worker_thread.joinable())
    {
        m_worker_thread.join();
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::updateDebugInfo()
{
    // Vérifier si nous sommes connectés et si la structure de l'arbre a été envoyée
    if (!m_connected || !m_tree_structure_sent)
    {
        std::cout << "Non connecté ou structure d'arbre non envoyée, ne mettant pas à jour les états" << std::endl;
        return;
    }

    // Créer une mise à jour d'état avec l'horodatage actuel
    StatusUpdate update;
    update.timestamp = std::chrono::system_clock::now();

    // Capturer l'état de tous les nœuds de l'arbre
    captureNodeStates(m_behavior_tree.getRoot(), update);

    // Si aucun état n'a été capturé, ne pas envoyer de mise à jour
    if (update.states.empty())
    {
        return;
    }

    // Ajouter la mise à jour à la file d'attente pour envoi par le thread worker
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_status_queue.push(std::move(update));
    }

    std::cout << "Mise à jour des états ajoutée à la file d'attente (" << update.states.size() << " nœuds)" << std::endl;
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::connect()
{
    std::cout << "Tentative de connexion à " << m_ip << ":" << m_port << std::endl;

    // Créer un socket TCP
    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        std::cerr << "Erreur lors de la création du socket: " << strerror(errno) << std::endl;
        m_connected = false;
        return;
    }

    // Configurer l'adresse du serveur
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_ip.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Adresse IP invalide: " << m_ip << std::endl;
        close(m_socket);
        m_socket = -1;
        m_connected = false;
        return;
    }

    // Se connecter au serveur
    std::cout << "Tentative de connexion au serveur..." << std::endl;
    if (::connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Échec de la connexion: " << strerror(errno) << std::endl;
        close(m_socket);
        m_socket = -1;
        m_connected = false;
        return;
    }

    std::cout << "Connexion établie avec succès!" << std::endl;
    m_connected = true;

    // Assigner des identifiants uniques aux nœuds en utilisant un parcours infixe
    uint32_t next_id = 0;
    assignNodeIds(m_behavior_tree.getRoot(), next_id);

    // Envoyer la structure de l'arbre
    sendTreeStructure();
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::sendTreeStructure()
{
    std::cout << "Envoi de la structure de l'arbre en YAML..." << std::endl;

    try {
        // Générer la représentation YAML de l'arbre
        std::string yaml_tree = TreeExporter::toYAML(m_behavior_tree);

        // Préparer le message
        std::vector<uint8_t> buffer;

        // Type de message
        buffer.push_back(static_cast<uint8_t>(MessageType::TREE_STRUCTURE));

        // Longueur de la chaîne YAML
        uint32_t yaml_length = static_cast<uint32_t>(yaml_tree.length());
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&yaml_length),
                     reinterpret_cast<const uint8_t*>(&yaml_length) + sizeof(yaml_length));

        // Contenu YAML
        buffer.insert(buffer.end(), yaml_tree.begin(), yaml_tree.end());

        // Envoyer le message
        ssize_t sent_bytes = send(m_socket, buffer.data(), buffer.size(), 0);
        if (sent_bytes <= 0) {
            std::cerr << "Erreur lors de l'envoi de la structure YAML: " << strerror(errno) << std::endl;
            m_connected = false;
            close(m_socket);
            m_socket = -1;
        } else {
            std::cout << "Structure de l'arbre envoyée en YAML (" << sent_bytes << " octets)" << std::endl;
            m_tree_structure_sent = true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception lors de la génération ou de l'envoi du YAML: " << e.what() << std::endl;
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

    // Parcours infixe: d'abord les enfants, puis le nœud courant

    // Traiter les enfants d'abord si c'est un nœud composite
    if (auto composite = std::dynamic_pointer_cast<Composite>(p_node)) {
        for (auto const& child : composite->getChildren()) {
            assignNodeIds(child, p_next_id);
        }
    }
    // Traiter l'enfant si c'est un nœud décorateur
    else if (auto decorator = std::dynamic_pointer_cast<Decorator>(p_node)) {
        if (decorator->getChild()) {
            assignNodeIds(decorator->getChild(), p_next_id);
        }
    }

    // Assigner un ID au nœud courant
    m_node_to_id[p_node.get()] = p_next_id++;

    std::cout << "Nœud '" << p_node->name << "' assigné à l'ID " << m_node_to_id[p_node.get()] << std::endl;
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::captureNodeStates(bt::Node::Ptr p_node, StatusUpdate& p_update)
{
    if (!p_node)
        return;

    // Obtenir l'ID du nœud
    auto it = m_node_to_id.find(p_node.get());
    if (it != m_node_to_id.end()) {
        // Ajouter l'état du nœud à la mise à jour
        p_update.states.emplace_back(it->second, p_node->getStatus());
    }

    // Parcourir les enfants de manière récursive

    // Traiter les enfants si c'est un nœud composite
    if (auto composite = std::dynamic_pointer_cast<Composite>(p_node)) {
        for (auto const& child : composite->getChildren()) {
            captureNodeStates(child, p_update);
        }
    }
    // Traiter l'enfant si c'est un nœud décorateur
    else if (auto decorator = std::dynamic_pointer_cast<Decorator>(p_node)) {
        if (decorator->getChild()) {
            captureNodeStates(decorator->getChild(), p_update);
        }
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::workerThread()
{
    std::cout << "Démarrage du thread worker" << std::endl;

    // Boucle principale du thread
    while (m_running)
    {
        // Si nous ne sommes pas connectés, essayer de se connecter
        if (!m_connected)
        {
            std::cout << "Non connecté, tentative de reconnexion..." << std::endl;
            connect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Récupérer toutes les mises à jour en attente
        std::vector<StatusUpdate> updates;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            while (!m_status_queue.empty())
            {
                updates.push_back(std::move(m_status_queue.front()));
                m_status_queue.pop();
            }
        }

        // Envoyer chaque mise à jour
        for (const auto& update : updates)
        {
            sendStatusUpdate(update);

            // Si l'envoi a échoué et que nous sommes déconnectés, sortir de la boucle
            if (!m_connected)
                break;
        }

        // Attendre un court instant avant la prochaine itération
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
    }

    std::cout << "Arrêt du thread worker" << std::endl;
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::sendStatusUpdate(const StatusUpdate& p_update)
{
    // Ne rien faire si la mise à jour est vide
    if (p_update.states.empty()) {
        return;
    }

    std::cout << "Envoi de la mise à jour des états pour " << p_update.states.size() << " nœuds" << std::endl;

    try {
        // Préparer le buffer pour la sérialisation
        std::vector<uint8_t> buffer;

        // Type de message
        buffer.push_back(static_cast<uint8_t>(MessageType::STATE_UPDATE));

        // Horodatage
        auto timestamp = std::chrono::system_clock::to_time_t(p_update.timestamp);
        buffer.insert(buffer.end(),
                     reinterpret_cast<uint8_t*>(&timestamp),
                     reinterpret_cast<uint8_t*>(&timestamp) + sizeof(timestamp));

        // Nombre de mises à jour d'état
        uint32_t count = static_cast<uint32_t>(p_update.states.size());
        buffer.insert(buffer.end(),
                     reinterpret_cast<uint8_t*>(&count),
                     reinterpret_cast<uint8_t*>(&count) + sizeof(count));

        // Ajouter chaque mise à jour d'état (ID, statut)
        for (const auto& [id, status] : p_update.states)
        {
            // ID du nœud
            buffer.insert(buffer.end(),
                         reinterpret_cast<const uint8_t*>(&id),
                         reinterpret_cast<const uint8_t*>(&id) + sizeof(id));

            // Statut sous forme d'uint8_t
            uint8_t status_value = static_cast<uint8_t>(status);
            buffer.push_back(status_value);
        }

        // Envoyer le message
        if (send(m_socket, buffer.data(), buffer.size(), 0) <= 0)
        {
            std::cerr << "Erreur lors de l'envoi de la mise à jour des états: " << strerror(errno) << std::endl;
            m_connected = false;
            close(m_socket);
            m_socket = -1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception lors de l'envoi de la mise à jour des états: " << e.what() << std::endl;
        m_connected = false;
        close(m_socket);
        m_socket = -1;
    }
}

} // namespace bt