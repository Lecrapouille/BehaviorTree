//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2025 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//*****************************************************************************

#include "NodeShape.hpp"

#include <cassert>
#include <cmath>

namespace bt {

// ----------------------------------------------------------------------------
NodeShape::NodeShape()
{
    // Initialize the base rectangle
    m_background.setFillColor(sf::Color(30, 30, 30));
    m_background.setOutlineThickness(0);

    // Initialize the gradient
    for (int i = 0; i < 4; ++i)
    {
        m_gradientVertices[i].color = sf::Color::Transparent;
    }

    // Initialize default colors
    m_mainColor = sf::Color(24, 35, 64);         // Dark blue (top)
    m_secondaryColor = sf::Color(16, 24, 45);    // Very dark blue (bottom)
    m_borderColor = sf::Color(0, 255, 255, 200); // Cyan semi-transparent

    // Do not initialize the text here, it will be created in setText()
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setCornerRadius(float radius)
{
    m_radius = radius;
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setText(const std::string& text,
                        const sf::Font& font,
                        unsigned int charSize)
{
    // Create a new text rather than modifying the existing one
    m_text = sf::Text();
    m_text.setString(text);
    m_text.setFont(font);
    m_text.setCharacterSize(charSize);

    // Force the white color and disable any outline
    m_text.setFillColor(sf::Color::White);
    m_text.setOutlineThickness(0);
    m_text.setOutlineColor(sf::Color::Transparent);
    m_text.setStyle(sf::Text::Regular);

    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setIcon(const sf::Texture& texture, float scale)
{
    m_icon.setTexture(texture);
    m_icon.setScale(scale, scale);
    m_icon.setColor(sf::Color::White);
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setColors(const sf::Color& mainColor,
                          const sf::Color& secondaryColor,
                          const sf::Color& borderColor)
{
    // Gradient colors and border color
    m_mainColor = mainColor;
    m_secondaryColor = secondaryColor;
    m_borderColor = borderColor;

    // Update gradient colors
    m_gradientVertices[0].color = mainColor;
    m_gradientVertices[1].color = mainColor;
    m_gradientVertices[2].color = secondaryColor;
    m_gradientVertices[3].color = secondaryColor;

    // Update rounded rectangle colors
    updateRoundedRectangle();
    updateBorder();
}

// ----------------------------------------------------------------------------
void NodeShape::setTextSmoothing(bool smooth)
{
    m_textSmoothing = smooth;

    // If the text exists, update its properties
    if (m_text.getFont() != nullptr)
    {
        // Adjust the text rendering quality
        if (smooth)
        {
            // Add a slight outline of the same color to improve smoothing
            m_text.setOutlineThickness(0.5f);
            m_text.setOutlineColor(sf::Color(255, 255, 255, 100));
        }
        else
        {
            m_text.setOutlineThickness(0);
        }
    }
}

// ----------------------------------------------------------------------------
void NodeShape::setPadding(float horizontal, float vertical)
{
    m_padding = {horizontal, vertical};
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    // Draw the rounded rectangle with gradient
    target.draw(m_roundedRectangle, states);

    // Draw the cyan border
    target.draw(m_border, states);

    // Draw the icon if available
    if (m_icon.getTexture())
    {
        target.draw(m_icon, states);
    }

    // Draw the text only if it has a valid font
    if (m_text.getFont())
    {
        target.draw(m_text, states);
    }
}

// ----------------------------------------------------------------------------
void NodeShape::updateGeometry()
{
    // Default dimensions if no text
    float width = 100.0f;
    float height = 40.0f;

    // Calculate dimensions if the text has a valid font
    if (m_text.getFont())
    {
        sf::FloatRect textBounds = m_text.getLocalBounds();
        sf::FloatRect iconBounds;

        // Take into account the icon only if a texture is defined
        float iconWidth = 0.0f;
        if (m_icon.getTexture())
        {
            iconBounds = m_icon.getGlobalBounds();
            iconWidth = iconBounds.width +
                        15.0f; // Espacement entre l'icône et le texte
        }

        // Calculate the width and height with appropriate spacing
        width = textBounds.width + iconWidth + m_padding.x * 2;
        height =
            std::max(textBounds.height, iconBounds.height) + m_padding.y * 2;

        // Minimum dimensions
        width = std::max(width, 100.0f);
        height = std::max(height, 40.0f);

        // Center the text vertically and position after the icon
        m_text.setOrigin(0, textBounds.top + textBounds.height / 2.0f);
        float textX = m_padding.x + iconWidth;
        float textY = height / 2.0f;
        m_text.setPosition(textX, textY);

        // Position the icon if available
        if (m_icon.getTexture())
        {
            m_icon.setOrigin(0, iconBounds.height / 2.0f);
            m_icon.setPosition(m_padding.x, height / 2.0f);
        }
    }

    m_currentSize = {width, height};

    // Update the rounded rectangle and the border
    updateRoundedRectangle();
    updateBorder();
}

// ----------------------------------------------------------------------------
void NodeShape::updateRoundedRectangle()
{
    float width = m_currentSize.x;
    float height = m_currentSize.y;
    float radius = m_radius;

    // Limit the radius to half of the smallest dimension
    radius = std::min(radius, std::min(width / 2.0f, height / 2.0f));

    // Reset the rounded rectangle
    m_roundedRectangle.clear();

    // Add the center point for the triangle fan
    m_roundedRectangle.append(
        sf::Vertex(sf::Vector2f(width / 2.0f, height / 2.0f),
                   interpolateColor(m_mainColor, m_secondaryColor, 0.5f)));

    // Number of segments for the rounded corners (higher = smoother)
    const int cornerSegments = 8;

    // Create the vertices for the rounded rectangle with gradient

    // Top left corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle =
            M_PIf + (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = radius + radius * cosf(angle);
        float y = radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_mainColor);
        m_roundedRectangle.append(vertex);
    }

    // Top edge
    m_roundedRectangle.append(sf::Vertex(sf::Vector2f(radius, 0), m_mainColor));
    m_roundedRectangle.append(
        sf::Vertex(sf::Vector2f(width - radius, 0), m_mainColor));

    // Top right corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle = 3.0f * M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = width - radius + radius * cosf(angle);
        float y = radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_mainColor);
        m_roundedRectangle.append(vertex);
    }

    // Right edge
    m_roundedRectangle.append(
        sf::Vertex(sf::Vector2f(width, radius), m_mainColor));
    m_roundedRectangle.append(
        sf::Vertex(sf::Vector2f(width, height - radius),
                   interpolateColor(m_mainColor, m_secondaryColor, 0.7f)));

    // Bottom right corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle =
            0 + (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = width - radius + radius * cosf(angle);
        float y = height - radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_secondaryColor);
        m_roundedRectangle.append(vertex);
    }

    // Bottom edge
    m_roundedRectangle.append(
        sf::Vertex(sf::Vector2f(width - radius, height), m_secondaryColor));
    m_roundedRectangle.append(
        sf::Vertex(sf::Vector2f(radius, height), m_secondaryColor));

    // Bottom left corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle = M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = radius + radius * cosf(angle);
        float y = height - radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_secondaryColor);
        m_roundedRectangle.append(vertex);
    }

    // Left edge
    m_roundedRectangle.append(
        sf::Vertex(sf::Vector2f(0, height - radius),
                   interpolateColor(m_mainColor, m_secondaryColor, 0.7f)));
    m_roundedRectangle.append(sf::Vertex(sf::Vector2f(0, radius), m_mainColor));

    // Close the contour
    float angle = M_PIf + (M_PIf / 2.0f) * 0 / cornerSegments;
    float x = radius + radius * cosf(angle);
    float y = radius + radius * sinf(angle);
    m_roundedRectangle.append(sf::Vertex(sf::Vector2f(x, y), m_mainColor));
}

// ----------------------------------------------------------------------------
void NodeShape::updateBorder()
{
    float width = m_currentSize.x;
    float height = m_currentSize.y;
    float radius = m_radius;

    // Limit the radius to half of the smallest dimension
    radius = std::min(radius, std::min(width / 2.0f, height / 2.0f));

    // Reset the border
    m_border.clear();

    // Use triangles to create a thicker border
    m_border.setPrimitiveType(sf::TriangleStrip);

    // Border thickness
    float borderThickness = 2.0f;

    // Number of segments for the rounded corners
    const int cornerSegments = 16; // More segments for a smoother border

    // Function to add two points to the border (inner and outer)
    auto addBorderPoints = [&](float x, float y, float nx, float ny) {
        // Inner point
        m_border.append(sf::Vertex(sf::Vector2f(x, y), m_borderColor));
        // Outer point
        m_border.append(sf::Vertex(
            sf::Vector2f(x + nx * borderThickness, y + ny * borderThickness),
            m_borderColor));
    };

    // Top left corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle =
            M_PIf + (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = radius + radius * cosf(angle);
        float y = radius + radius * sinf(angle);
        float nx = cosf(angle); // Normal x
        float ny = sinf(angle); // Normal y
        addBorderPoints(x, y, nx, ny);
    }

    // Top edge
    addBorderPoints(radius, 0, 0, -1);
    addBorderPoints(width - radius, 0, 0, -1);

    // Top right corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle = 3.0f * M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = width - radius + radius * cosf(angle);
        float y = radius + radius * sinf(angle);
        float nx = cosf(angle); // Normal x
        float ny = sinf(angle); // Normale y
        addBorderPoints(x, y, nx, ny);
    }

    // Right edge
    addBorderPoints(width, radius, 1, 0);
    addBorderPoints(width, height - radius, 1, 0);

    // Bottom right corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle =
            0 + (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = width - radius + radius * cosf(angle);
        float y = height - radius + radius * sinf(angle);
        float nx = cosf(angle); // Normal x
        float ny = sinf(angle); // Normal y
        addBorderPoints(x, y, nx, ny);
    }

    // Bottom edge
    addBorderPoints(width - radius, height, 0, 1);
    addBorderPoints(radius, height, 0, 1);

    // Bottom left corner
    for (int i = 0; i <= cornerSegments; ++i)
    {
        float angle = M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / cornerSegments;
        float x = radius + radius * cosf(angle);
        float y = height - radius + radius * sinf(angle);
        float nx = cosf(angle); // Normal x
        float ny = sinf(angle); // Normal y
        addBorderPoints(x, y, nx, ny);
    }

    // Left edge
    addBorderPoints(0, height - radius, -1, 0);
    addBorderPoints(0, radius, -1, 0);

    // Close the border by returning to the first point
    float closeAngle = M_PIf + (M_PIf / 2.0f) * 0 / cornerSegments;
    float closeX = radius + radius * cosf(closeAngle);
    float closeY = radius + radius * sinf(closeAngle);
    float closeNx = cosf(closeAngle);
    float closeNy = sinf(closeAngle);
    addBorderPoints(closeX, closeY, closeNx, closeNy);
}

// ----------------------------------------------------------------------------
sf::Color NodeShape::interpolateColor(const sf::Color& color1,
                                      const sf::Color& color2,
                                      float factor) const
{
    return sf::Color(
        static_cast<sf::Uint8>(color1.r + (color2.r - color1.r) * factor),
        static_cast<sf::Uint8>(color1.g + (color2.g - color1.g) * factor),
        static_cast<sf::Uint8>(color1.b + (color2.b - color1.b) * factor),
        static_cast<sf::Uint8>(color1.a + (color2.a - color1.a) * factor));
}

} // namespace bt
