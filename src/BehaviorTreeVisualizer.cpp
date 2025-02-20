#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

namespace bt
{

// ----------------------------------------------------------------------------
BehaviorTreeVisualizer::BehaviorTreeVisualizer(bt::Tree& p_bt, std::string p_ip, uint16_t p_port)
    : m_behavior_tree(p_bt), m_ip(p_ip), m_port(p_port)
{
    m_worker_thread = std::thread(&BehaviorTreeVisualizer::workerThread, this);
}

// ----------------------------------------------------------------------------
BehaviorTreeVisualizer::~BehaviorTreeVisualizer()
{
    m_running = false;
    
    if (m_socket >= 0)
    {
        close(m_socket);
        m_socket = -1;
    }

    if (m_worker_thread.joinable())
    {
        m_worker_thread.join();
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::updateDebugInfo()
{
    if (!m_connected || !m_tree_structure_sent)
    {
        std::cout << "Non connecté ou structure d'arbre non envoyée, ne mettant pas à jour les états" << std::endl;
        return;
    }

    std::cout << "Mise à jour des états" << std::endl;
    StatusUpdate update;
    update.timestamp = std::chrono::system_clock::now();
    captureNodeStates(m_behavior_tree.getRoot(), update);

    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_status_queue.push(std::move(update));
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::connect()
{
    std::cout << "Tentative de connexion à " << m_ip << ":" << m_port << std::endl;
    
    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        std::cerr << "Erreur lors de la création du socket: " << strerror(errno) << std::endl;
        m_connected = false;
        return;
    }

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
    sendTreeStructure();
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::sendTreeStructure()
{
    std::cout << "Envoi de la structure de l'arbre..." << std::endl;
    
    std::vector<NodeStructure> nodes;
    uint32_t next_id = 0;
    buildTreeStructure(m_behavior_tree.getRoot(), nodes, next_id, uint32_t(-1));

    // Serialization
    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(MessageType::TREE_STRUCTURE));

    // Number of nodes
    size_t count = nodes.size();
    buffer.insert(buffer.end(),
                 reinterpret_cast<uint8_t*>(&count),
                 reinterpret_cast<uint8_t*>(&count) + sizeof(count));

    // Data of each node
    for (const auto& node : nodes)
    {
        // ID
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&node.id),
                     reinterpret_cast<const uint8_t*>(&node.id) + sizeof(node.id));

        // Name
        size_t name_length = node.name.length();
        buffer.insert(buffer.end(),
                     reinterpret_cast<uint8_t*>(&name_length),
                     reinterpret_cast<uint8_t*>(&name_length) + sizeof(name_length));
        buffer.insert(buffer.end(), node.name.begin(), node.name.end());

        // Parent ID
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&node.parent_id),
                     reinterpret_cast<const uint8_t*>(&node.parent_id) + sizeof(node.parent_id));

        // Children IDs
        size_t child_count = node.children_ids.size();
        buffer.insert(buffer.end(),
                     reinterpret_cast<uint8_t*>(&child_count),
                     reinterpret_cast<uint8_t*>(&child_count) + sizeof(child_count));
        for (uint32_t child_id : node.children_ids)
        {
            buffer.insert(buffer.end(),
                         reinterpret_cast<const uint8_t*>(&child_id),
                         reinterpret_cast<const uint8_t*>(&child_id) + sizeof(child_id));
        }
    }

    ssize_t sent_bytes = send(m_socket, buffer.data(), buffer.size(), 0);
    if (sent_bytes <= 0) {
        std::cerr << "Erreur lors de l'envoi de la structure: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Structure de l'arbre envoyée (" << sent_bytes << " octets)" << std::endl;
    }
    m_tree_structure_sent = true;
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::buildTreeStructure(bt::Node::Ptr p_node,
                                                std::vector<NodeStructure>& p_nodes,
                                                uint32_t& p_next_id,
                                                uint32_t p_parent_id)
{
    std::cout << "Construction de la structure de l'arbre" << std::endl;

    NodeStructure node_struct;
    node_struct.id = p_next_id++;
    node_struct.name = "Node_" + std::to_string(node_struct.id);
    node_struct.parent_id = p_parent_id;

    m_node_to_id[p_node.get()] = node_struct.id;

    std::vector<bt::Node::Ptr> children;
    for (const auto& child : children)
    {
        uint32_t child_id = p_next_id;
        node_struct.children_ids.push_back(child_id);
        buildTreeStructure(child, p_nodes, p_next_id, node_struct.id);
    }

    p_nodes.push_back(std::move(node_struct));
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::captureNodeStates(bt::Node::Ptr p_node, StatusUpdate& p_update)
{
    std::cout << "Capture des états du noeud" << std::endl;

    auto it = m_node_to_id.find(p_node.get());
    if (it != m_node_to_id.end())
    {
        p_update.states.emplace_back(it->second, bt::Status(0)); // INVALID
    }

    std::vector<bt::Node::Ptr> children;
    for (const auto& child : children)
    {
        captureNodeStates(child, p_update);
    }
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::workerThread()
{
    std::cout << "Démarrage du thread worker" << std::endl;
    
    while (m_running)
    {
        if (!m_connected)
        {
            std::cout << "Non connecté, tentative de reconnexion..." << std::endl;
            connect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        std::vector<StatusUpdate> updates;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            while (!m_status_queue.empty())
            {
                updates.push_back(std::move(m_status_queue.front()));
                m_status_queue.pop();
            }
        }

        for (const auto& update : updates)
        {
            sendStatusUpdate(update);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
    }
    
    std::cout << "Arrêt du thread worker" << std::endl;
}

// ----------------------------------------------------------------------------
void BehaviorTreeVisualizer::sendStatusUpdate(const StatusUpdate& p_update)
{
    std::cout << "Envoi de la mise à jour des états" << std::endl;

    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(MessageType::STATE_UPDATE));

    auto timestamp = std::chrono::system_clock::to_time_t(p_update.timestamp);
    buffer.insert(buffer.end(),
                 reinterpret_cast<uint8_t*>(&timestamp),
                 reinterpret_cast<uint8_t*>(&timestamp) + sizeof(timestamp));

    size_t count = p_update.states.size();
    buffer.insert(buffer.end(),
                 reinterpret_cast<uint8_t*>(&count),
                 reinterpret_cast<uint8_t*>(&count) + sizeof(count));

    for (const auto& [id, status] : p_update.states)
    {
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&id),
                     reinterpret_cast<const uint8_t*>(&id) + sizeof(id));
        buffer.push_back(static_cast<uint8_t>(status));
    }

    if (send(m_socket, buffer.data(), buffer.size(), 0) <= 0)
    {
        m_connected = false;
        close(m_socket);
        m_socket = -1;
    }
}

} // namespace bt