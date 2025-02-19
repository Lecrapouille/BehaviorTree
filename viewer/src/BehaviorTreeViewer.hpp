#pragma once

#include "DebugProtocol.hpp"
#include "raylib.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

class NodeRenderer;
class DebugServer;

class BehaviorTreeViewer {
public:
    BehaviorTreeViewer();
    ~BehaviorTreeViewer();

    /// @brief Initialise la fenêtre de visualisation
    /// @param[in] width Largeur de la fenêtre
    /// @param[in] height Hauteur de la fenêtre
    /// @return true si l'initialisation est réussie
    bool initialize(int width = 1280, int height = 720);
    
    /// @brief Lance la boucle principale de visualisation
    void run();

private:
    void updateVisualization();
    void handleConnection();
    void updateLayout();  // Ajout d'une méthode pour gérer la disposition
    void draw();

    struct NodeInfo {
        std::string name;
        bt_debug::NodeStatus status;
        uint32_t parentId;
        std::vector<uint32_t> childrenIds;
        Vector2 position;
    };

    std::unique_ptr<DebugServer> server;
    std::unique_ptr<NodeRenderer> renderer;
    std::unordered_map<uint32_t, NodeInfo> nodes;
    
    bool treeReceived{false};
    int width{0};
    int height{0};
    Camera2D camera{};
    Font font{};
}; 