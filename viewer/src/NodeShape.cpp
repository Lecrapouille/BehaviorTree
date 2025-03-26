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
    for (auto& vertex : m_gradient_vertices)
    {
        vertex.color = sf::Color::Transparent;
    }

    // Initialize default colors
    m_main_color = sf::Color(24, 35, 64);         // Dark blue (top)
    m_secondary_color = sf::Color(16, 24, 45);    // Very dark blue (bottom)
    m_border_color = sf::Color(0, 255, 255, 200); // Cyan semi-transparent

    // Do not initialize the text here, it will be created in setText()
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setCornerRadius(float p_radius)
{
    m_radius = p_radius;
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setText(const std::string& p_text,
                        const sf::Font& p_font,
                        unsigned int p_char_size)
{
    // Create a new text rather than modifying the existing one
    m_text = sf::Text();
    m_text.setString(p_text);
    m_text.setFont(p_font);
    m_text.setCharacterSize(p_char_size);

    // Force the white color and disable any outline
    m_text.setFillColor(sf::Color::White);
    m_text.setOutlineThickness(0);
    m_text.setOutlineColor(sf::Color::Transparent);
    m_text.setStyle(sf::Text::Regular);

    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setIcon(const sf::Texture& p_texture, float p_scale)
{
    m_icon.setTexture(p_texture);
    m_icon.setScale(p_scale, p_scale);
    m_icon.setColor(sf::Color::White);
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::setColors(const sf::Color& p_main_color,
                          const sf::Color& p_secondary_color,
                          const sf::Color& p_border_color)
{
    // Gradient colors and border color
    m_main_color = p_main_color;
    m_secondary_color = p_secondary_color;
    m_border_color = p_border_color;

    // Update gradient colors
    m_gradient_vertices[0].color = p_main_color;
    m_gradient_vertices[1].color = p_main_color;
    m_gradient_vertices[2].color = p_secondary_color;
    m_gradient_vertices[3].color = p_secondary_color;

    // Update rounded rectangle colors
    updateRoundedRectangle();
    updateBorder();
}

// ----------------------------------------------------------------------------
void NodeShape::setTextSmoothing(bool p_smooth)
{
    m_text_smoothing = p_smooth;

    // If the text exists, update its properties
    if (m_text.getFont() != nullptr)
    {
        // Adjust the text rendering quality
        if (p_smooth)
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
void NodeShape::setPadding(float p_horizontal, float p_vertical)
{
    m_padding = {p_horizontal, p_vertical};
    updateGeometry();
}

// ----------------------------------------------------------------------------
void NodeShape::draw(sf::RenderTarget& p_target,
                     sf::RenderStates p_states) const
{
    p_states.transform *= getTransform();

    // Draw the rounded rectangle with gradient
    p_target.draw(m_rounded_rectangle, p_states);

    // Draw the cyan border
    p_target.draw(m_border, p_states);

    // Draw the icon if available
    if (m_icon.getTexture())
    {
        p_target.draw(m_icon, p_states);
    }

    // Draw the text only if it has a valid font
    if (m_text.getFont())
    {
        p_target.draw(m_text, p_states);
    }
}

// ----------------------------------------------------------------------------
void NodeShape::updateGeometry()
{
    // Space between the icon and the text
    const float ICON_TEXT_SPACING = 15.0f;

    // Default dimensions if no text
    const float MIN_WIDTH = 100.0f;
    const float MIN_HEIGHT = 40.0f;

    float width = MIN_WIDTH;
    float height = MIN_HEIGHT;

    // Calculate dimensions if the text has a valid font
    if (m_text.getFont())
    {
        sf::FloatRect text_bounds = m_text.getLocalBounds();
        sf::FloatRect icon_bounds;

        // Take into account the icon only if a texture is defined
        float icon_width = 0.0f;
        if (m_icon.getTexture())
        {
            icon_bounds = m_icon.getGlobalBounds();
            icon_width = icon_bounds.width + ICON_TEXT_SPACING;
        }

        // Calculate the width and height with appropriate spacing
        width = text_bounds.width + icon_width + m_padding.x * 2.0f;
        height = std::max(text_bounds.height, icon_bounds.height);
        height += m_padding.y * 2.0f;

        // Minimum dimensions
        width = std::max(width, MIN_WIDTH);
        height = std::max(height, MIN_HEIGHT);

        // Center the text vertically and position after the icon
        m_text.setOrigin(0, text_bounds.top + text_bounds.height / 2.0f);
        float text_x = m_padding.x + icon_width;
        float text_y = height / 2.0f;
        m_text.setPosition(text_x, text_y);

        // Position the icon if available
        if (m_icon.getTexture())
        {
            m_icon.setOrigin(icon_bounds.width / 2.0f,
                             icon_bounds.height / 2.0f);
            m_icon.setPosition(m_padding);
        }
    }

    m_current_size = {width, height};

    // Update the rounded rectangle and the border
    updateRoundedRectangle();
    updateBorder();
}

// ----------------------------------------------------------------------------
void NodeShape::updateRoundedRectangle()
{
    float width = m_current_size.x;
    float height = m_current_size.y;
    float radius = m_radius;

    // Limit the radius to half of the smallest dimension
    radius = std::min(radius, std::min(width / 2.0f, height / 2.0f));

    // Reset the rounded rectangle
    m_rounded_rectangle.clear();

    // Add the center point for the triangle fan
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(width / 2.0f, height / 2.0f),
                   interpolateColor(m_main_color, m_secondary_color, 0.5f)));

    // Number of segments for the rounded corners (higher = smoother)
    const int corner_segments = 8;

    // Create the vertices for the rounded rectangle with gradient

    // Top left corner
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle =
            M_PIf + (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
        float x = radius + radius * cosf(angle);
        float y = radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_main_color);
        m_rounded_rectangle.append(vertex);
    }

    // Top edge
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(radius, 0), m_main_color));
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(width - radius, 0), m_main_color));

    // Top right corner
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle = 3.0f * M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
        float x = width - radius + radius * cosf(angle);
        float y = radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_main_color);
        m_rounded_rectangle.append(vertex);
    }

    // Right edge
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(width, radius), m_main_color));
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(width, height - radius),
                   interpolateColor(m_main_color, m_secondary_color, 0.7f)));

    // Bottom right corner
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle =
            0 + (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
        float x = width - radius + radius * cosf(angle);
        float y = height - radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_secondary_color);
        m_rounded_rectangle.append(vertex);
    }

    // Bottom edge
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(width - radius, height), m_secondary_color));
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(radius, height), m_secondary_color));

    // Bottom left corner
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle = M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
        float x = radius + radius * cosf(angle);
        float y = height - radius + radius * sinf(angle);
        sf::Vertex vertex(sf::Vector2f(x, y), m_secondary_color);
        m_rounded_rectangle.append(vertex);
    }

    // Left edge
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(0, height - radius),
                   interpolateColor(m_main_color, m_secondary_color, 0.7f)));
    m_rounded_rectangle.append(
        sf::Vertex(sf::Vector2f(0, radius), m_main_color));

    // Close the contour
    float angle = M_PIf + (M_PIf / 2.0f) * 0 / corner_segments;
    float x = radius + radius * cosf(angle);
    float y = radius + radius * sinf(angle);
    m_rounded_rectangle.append(sf::Vertex(sf::Vector2f(x, y), m_main_color));
}

// ----------------------------------------------------------------------------
void NodeShape::updateBorder()
{
    float width = m_current_size.x;
    float height = m_current_size.y;
    float radius = m_radius;

    // Limit the radius to half of the smallest dimension
    radius = std::min(radius, std::min(width / 2.0f, height / 2.0f));

    // Reset the border
    m_border.clear();

    // Use triangles to create a thicker border
    m_border.setPrimitiveType(sf::TriangleStrip);

    // Border thickness
    float border_thickness = 2.0f;

    // Number of segments for the rounded corners
    const int corner_segments = 16; // More segments for a smoother border

    // Function to add two points to the border (inner and outer)
    auto addBorderPoints = [&](float x, float y, float nx, float ny) {
        // Inner point
        m_border.append(sf::Vertex(sf::Vector2f(x, y), m_border_color));
        // Outer point
        m_border.append(sf::Vertex(
            sf::Vector2f(x + nx * border_thickness, y + ny * border_thickness),
            m_border_color));
    };

    // Top left corner
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle =
            M_PIf + (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
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
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle = 3.0f * M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
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
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle =
            0 + (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
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
    for (int i = 0; i <= corner_segments; ++i)
    {
        float angle = M_PIf / 2.0f +
                      (M_PIf / 2.0f) * static_cast<float>(i) / corner_segments;
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
    float close_angle = M_PIf + (M_PIf / 2.0f) * 0 / corner_segments;
    float close_x = radius + radius * cosf(close_angle);
    float close_y = radius + radius * sinf(close_angle);
    float close_nx = cosf(close_angle);
    float close_ny = sinf(close_angle);
    addBorderPoints(close_x, close_y, close_nx, close_ny);
}

// ----------------------------------------------------------------------------
sf::Color NodeShape::interpolateColor(const sf::Color& p_color1,
                                      const sf::Color& p_color2,
                                      float p_factor) const
{
    return {static_cast<sf::Uint8>(p_color1.r +
                                   (p_color2.r - p_color1.r) * p_factor),
            static_cast<sf::Uint8>(p_color1.g +
                                   (p_color2.g - p_color1.g) * p_factor),
            static_cast<sf::Uint8>(p_color1.b +
                                   (p_color2.b - p_color1.b) * p_factor),
            static_cast<sf::Uint8>(p_color1.a +
                                   (p_color2.a - p_color1.a) * p_factor)};
}

} // namespace bt
