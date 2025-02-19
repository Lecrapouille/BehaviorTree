#include "BehaviorTreeViewer.hpp"
#include "NodeRenderer.hpp"
#include "DebugServer.hpp"

BehaviorTreeViewer::BehaviorTreeViewer()
    : treeReceived(false)
    , width(0)
    , height(0)
{
    camera = { 0 };
    camera.zoom = 1.0f;
}

void BehaviorTreeViewer::run()
{
    while (!WindowShouldClose()) {
        updateVisualization();
        
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            if (GetMouseWheelMove() > 0) camera.zoom *= 1.1f;
            if (GetMouseWheelMove() < 0) camera.zoom /= 1.1f;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        BeginMode2D(camera);
            draw();
        EndMode2D();

        DrawText("Use middle mouse to pan, Ctrl+wheel to zoom", 10, 10, 20, DARKGRAY);
        
        EndDrawing();
    }
}

void BehaviorTreeViewer::draw()
{
    if (!treeReceived) return;

    // Trouve la racine
    auto root = std::find_if(nodes.begin(), nodes.end(),
        [](const auto& pair) { return pair.second.parentId == -1; });

    if (root != nodes.end()) {
        updateLayout();
        
        // Dessine les connexions d'abord
        for (const auto& [id, node] : nodes) {
            for (uint32_t childId : node.childrenIds) {
                auto childIt = nodes.find(childId);
                if (childIt != nodes.end()) {
                    renderer->DrawConnection(
                        node.position,
                        childIt->second.position
                    );
                }
            }
        }

        // Puis dessine les nœuds
        for (const auto& [id, node] : nodes) {
            renderer->renderNode(
                node.name.c_str(),
                node.status,
                node.position,
                font
            );
        }
    }
}

void BehaviorTreeViewer::updateVisualization() {
    if (server && server->isConnected()) {
        // ... existing code ...
    }
} 