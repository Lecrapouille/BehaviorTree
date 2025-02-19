#pragma once

#include "DebugProtocol.hpp"
#include "raylib.h"

class NodeRenderer {
public:
    static constexpr float NODE_WIDTH = 150.0f;
    static constexpr float NODE_HEIGHT = 40.0f;
    static constexpr float VERTICAL_SPACING = 60.0f;
    static constexpr float HORIZONTAL_SPACING = 30.0f;

    NodeRenderer();

    void renderNode(const char* name, bt_debug::NodeStatus status, 
                   Vector2 position, const Font& font);
    void drawConnection(Vector2 start, Vector2 end);

private:
    Color getStatusColor(bt_debug::NodeStatus status) const;
}; 