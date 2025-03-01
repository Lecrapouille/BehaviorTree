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

    bool loadIcons(const std::string& p_path);
    void renderNode(const char* p_name, bt::Status p_status,
                    sf::Vector2f p_position, const sf::Font& p_font,
                    sf::RenderWindow& p_window);
    void drawConnection(sf::Vector2f p_start, sf::Vector2f p_end,
                        sf::RenderWindow& p_window);

private:
    sf::Color getStatusColor(bt::Status p_status) const;

private:
    sf::Texture m_icon;
};