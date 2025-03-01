#include "NodeRenderer.hpp"
#include "NodeShape.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
// ----------------------------------------------------------------------------
bool NodeRenderer::loadIcons(const std::string& p_path)
{
    if (!m_icon.loadFromFile(p_path + "/icon.png"))
    {
        std::cerr << "Failed to load icon texture" << std::endl;
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
void NodeRenderer::renderNode(const char* p_name, bt::Status p_status,
                              sf::Vector2f p_position, const sf::Font& p_font,
                              sf::RenderWindow& p_window)
{
    // Calculate the node width based on the length of the name
    float name_length = std::strlen(p_name);
    float node_width = std::max(NODE_WIDTH, name_length * 12.0f);

    // Create the node rectangle
    NodeShape rect;
    rect.setCornerRadius(10.0f);
    rect.setPosition(p_position.x - node_width / 2, p_position.y - NODE_HEIGHT / 2);
    
    // Définir les couleurs avec la couleur principale et secondaire identiques
    sf::Color statusColor = getStatusColor(p_status);
    rect.setColors(statusColor, statusColor);
    
    // Définir le texte avec la police fournie
    rect.setText(p_name, p_font, 20);
    
    // Définir l'icône si disponible
    rect.setIcon(m_icon);
    
    // Définir les marges internes pour un meilleur espacement
    rect.setPadding(15.0f, 12.0f);

    // Draw the rectangle (qui contient déjà le texte et l'icône)
    p_window.draw(rect);
}

// ----------------------------------------------------------------------------
void NodeRenderer::drawConnection(sf::Vector2f p_start, sf::Vector2f p_end,
                                  sf::RenderWindow& p_window)
{
    // Draw the line
    sf::RectangleShape line;
    float length = std::sqrt(
        (p_end.x - p_start.x) * (p_end.x - p_start.x) +
        (p_end.y - p_start.y) * (p_end.y - p_start.y)
    );
    line.setSize(sf::Vector2f(length, 2.0f));
    line.setPosition(p_start);
    line.setFillColor(sf::Color(64, 64, 64)); // DARKGRAY

    // Rotate the line to point to the end
    float angle = std::atan2(p_end.y - p_start.y, p_end.x - p_start.x) * 180.0f / M_PIf;
    line.setRotation(angle);

    p_window.draw(line);

    // Draw the arrow
    float arrow_size = 10.0f;
    float arrow_angle = 30.0f;

    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    arrow.setPoint(0, p_end);
    arrow.setPoint(1, sf::Vector2f(
        p_end.x - arrow_size * std::cos((angle - arrow_angle) * M_PIf / 180.0f),
        p_end.y - arrow_size * std::sin((angle - arrow_angle) * M_PIf / 180.0f)
    ));
    arrow.setPoint(2, sf::Vector2f(
        p_end.x - arrow_size * std::cos((angle + arrow_angle) * M_PIf / 180.0f),
        p_end.y - arrow_size * std::sin((angle + arrow_angle) * M_PIf / 180.0f)
    ));

    arrow.setFillColor(sf::Color(64, 64, 64)); // DARKGRAY
    p_window.draw(arrow);
}

// ----------------------------------------------------------------------------
sf::Color NodeRenderer::getStatusColor(bt::Status p_status) const
{
    switch (p_status)
    {
        case bt::Status::SUCCESS:
            return sf::Color(0, 255, 0);      // GREEN
        case bt::Status::FAILURE:
            return sf::Color(255, 0, 0);      // RED
        case bt::Status::RUNNING:
            return sf::Color(255, 255, 0);    // YELLOW
        default:
            return sf::Color(211, 211, 211);  // LIGHTGRAY
    }
}