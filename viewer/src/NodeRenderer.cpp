#include "NodeRenderer.hpp"
#include <cmath>
#include <cstring>

// ----------------------------------------------------------------------------
void NodeRenderer::renderNode(const char* name, bt::Status status,
                              sf::Vector2f position, const sf::Font& font,
                              sf::RenderWindow& window)
{
    // Calculate the node width based on the length of the name
    float name_length = std::strlen(name);
    float node_width = std::max(NODE_WIDTH, name_length * 12.0f);

    // Create the node rectangle
    sf::RectangleShape rect;
    rect.setSize(sf::Vector2f(node_width, NODE_HEIGHT));
    rect.setPosition(position.x - node_width / 2, position.y - NODE_HEIGHT / 2);
    rect.setFillColor(getStatusColor(status));
    rect.setOutlineThickness(2.0f);
    rect.setOutlineColor(sf::Color::Black);

    // Draw the rectangle
    window.draw(rect);

    // Create and draw the centered text
    sf::Text text;
    text.setFont(font);
    text.setString(name);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::Black);

    // Center the text
    sf::FloatRect text_bounds = text.getLocalBounds();
    text.setPosition(
        position.x - text_bounds.width / 2,
        position.y - text_bounds.height / 2
    );

    window.draw(text);
}

// ----------------------------------------------------------------------------
void NodeRenderer::drawConnection(sf::Vector2f start, sf::Vector2f end,
                                  sf::RenderWindow& window)
{
    // Draw the line
    sf::RectangleShape line;
    float length = std::sqrt(
        (end.x - start.x) * (end.x - start.x) +
        (end.y - start.y) * (end.y - start.y)
    );
    line.setSize(sf::Vector2f(length, 2.0f));
    line.setPosition(start);
    line.setFillColor(sf::Color(64, 64, 64)); // DARKGRAY

    // Rotate the line to point to the end
    float angle = std::atan2(end.y - start.y, end.x - start.x) * 180.0f / M_PIf;
    line.setRotation(angle);

    window.draw(line);

    // Draw the arrow
    float arrow_size = 10.0f;
    float arrow_angle = 30.0f;

    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    arrow.setPoint(0, end);
    arrow.setPoint(1, sf::Vector2f(
        end.x - arrow_size * std::cos((angle - arrow_angle) * M_PIf / 180.0f),
        end.y - arrow_size * std::sin((angle - arrow_angle) * M_PIf / 180.0f)
    ));
    arrow.setPoint(2, sf::Vector2f(
        end.x - arrow_size * std::cos((angle + arrow_angle) * M_PIf / 180.0f),
        end.y - arrow_size * std::sin((angle + arrow_angle) * M_PIf / 180.0f)
    ));

    arrow.setFillColor(sf::Color(64, 64, 64)); // DARKGRAY
    window.draw(arrow);
}

// ----------------------------------------------------------------------------
sf::Color NodeRenderer::getStatusColor(bt::Status status) const
{
    switch (status)
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