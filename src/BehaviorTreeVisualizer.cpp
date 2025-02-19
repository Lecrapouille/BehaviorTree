#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

BehaviorTreeVisualizer::BehaviorTreeVisualizer(BehaviorTree& bt)
    : behaviorTree(bt)
{
    workerThread = std::thread(&BehaviorTreeVisualizer::workerThread, this);
}

BehaviorTreeVisualizer::~BehaviorTreeVisualizer()
{
    running = false;
    if (socket >= 0) {
        close(socket);
        socket = -1;
    }
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void BehaviorTreeVisualizer::updateDebugInfo()
{
    if (!connected || !treeStructureSent) {
        return;
    }

    StatusUpdate update;
    update.timestamp = std::chrono::system_clock::now();
    captureNodeStates(behaviorTree.getRootNode(), update);

    std::lock_guard<std::mutex> lock(queueMutex);
    statusQueue.push(std::move(update));
}

void BehaviorTreeVisualizer::connect()
{
    socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        connected = false;
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9090);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (::connect(socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(socket);
        socket = -1;
        connected = false;
        return;
    }

    connected = true;
    sendTreeStructure();
}

void BehaviorTreeVisualizer::sendTreeStructure()
{
    std::vector<NodeStructure> nodes;
    uint32_t nextId = 0;
    buildTreeStructure(behaviorTree.getRootNode(), nodes, nextId);

    // Sérialisation
    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(MessageType::treeStructure));
    
    // Nombre de nœuds
    uint32_t count = nodes.size();
    buffer.insert(buffer.end(),
                 reinterpret_cast<uint8_t*>(&count),
                 reinterpret_cast<uint8_t*>(&count) + sizeof(count));

    // Données de chaque nœud
    for (const auto& node : nodes) {
        // ID
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&node.id),
                     reinterpret_cast<const uint8_t*>(&node.id) + sizeof(node.id));
        
        // Nom
        uint16_t nameLength = node.name.length();
        buffer.insert(buffer.end(),
                     reinterpret_cast<uint8_t*>(&nameLength),
                     reinterpret_cast<uint8_t*>(&nameLength) + sizeof(nameLength));
        buffer.insert(buffer.end(), node.name.begin(), node.name.end());
        
        // Parent ID
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&node.parentId),
                     reinterpret_cast<const uint8_t*>(&node.parentId) + sizeof(node.parentId));
        
        // Children IDs
        uint16_t childCount = node.childrenIds.size();
        buffer.insert(buffer.end(),
                     reinterpret_cast<uint8_t*>(&childCount),
                     reinterpret_cast<uint8_t*>(&childCount) + sizeof(childCount));
        for (uint32_t childId : node.childrenIds) {
            buffer.insert(buffer.end(),
                         reinterpret_cast<const uint8_t*>(&childId),
                         reinterpret_cast<const uint8_t*>(&childId) + sizeof(childId));
        }
    }

    send(socket, buffer.data(), buffer.size(), 0);
    treeStructureSent = true;
}

void BehaviorTreeVisualizer::buildTreeStructure(BehaviorNode* node, 
                                              std::vector<NodeStructure>& nodes,
                                              uint32_t& nextId, 
                                              uint32_t parentId)
{
    if (!node) return;

    NodeStructure nodeStruct;
    nodeStruct.id = nextId++;
    nodeStruct.name = node->getName();
    nodeStruct.parentId = parentId;
    
    nodeToId[node] = nodeStruct.id;

    for (auto* child : node->getChildren()) {
        uint32_t childId = nextId;
        nodeStruct.childrenIds.push_back(childId);
        buildTreeStructure(child, nodes, nextId, nodeStruct.id);
    }

    nodes.push_back(std::move(nodeStruct));
}

void BehaviorTreeVisualizer::captureNodeStates(BehaviorNode* node, StatusUpdate& update)
{
    if (!node) return;

    auto it = nodeToId.find(node);
    if (it != nodeToId.end()) {
        update.states.emplace_back(it->second, node->getStatus());
    }

    for (auto* child : node->getChildren()) {
        captureNodeStates(child, update);
    }
}

void BehaviorTreeVisualizer::workerThread()
{
    while (running) {
        if (!connected) {
            connect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        std::vector<StatusUpdate> updates;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            while (!statusQueue.empty()) {
                updates.push_back(std::move(statusQueue.front()));
                statusQueue.pop();
            }
        }

        for (const auto& update : updates) {
            sendStatusUpdate(update);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
    }
}

void BehaviorTreeVisualizer::sendStatusUpdate(const StatusUpdate& update)
{
    if (!connected) return;

    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(MessageType::stateUpdate));
    
    auto timestamp = std::chrono::system_clock::to_time_t(update.timestamp);
    buffer.insert(buffer.end(), 
                 reinterpret_cast<uint8_t*>(&timestamp),
                 reinterpret_cast<uint8_t*>(&timestamp) + sizeof(timestamp));

    uint32_t count = update.states.size();
    buffer.insert(buffer.end(),
                 reinterpret_cast<uint8_t*>(&count),
                 reinterpret_cast<uint8_t*>(&count) + sizeof(count));

    for (const auto& [id, status] : update.states) {
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&id),
                     reinterpret_cast<const uint8_t*>(&id) + sizeof(id));
        buffer.push_back(static_cast<uint8_t>(status));
    }

    if (send(socket, buffer.data(), buffer.size(), 0) <= 0) {
        connected = false;
        close(socket);
        socket = -1;
    }
} 