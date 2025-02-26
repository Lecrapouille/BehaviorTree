#pragma once

#include <SFML/Graphics.hpp>
#include "BehaviorTree/BehaviorTreeVisualizer.hpp"

// ****************************************************************************
//! \brief Classe pour rendre les nœuds de l'arbre de comportement
// ****************************************************************************
class NodeRenderer
{
public:
    static constexpr float NODE_WIDTH = 150.0f;
    static constexpr float NODE_HEIGHT = 40.0f;
    static constexpr float VERTICAL_SPACING = 60.0f;
    static constexpr float HORIZONTAL_SPACING = 50.0f;

    void renderNode(const char* name, bt::Status status,
                    sf::Vector2f position, const sf::Font& font,
                    sf::RenderWindow& window);
    void drawConnection(sf::Vector2f start, sf::Vector2f end,
                        sf::RenderWindow& window);

private:
    sf::Color getStatusColor(bt::Status status) const;
};