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

#include "ArcShape.hpp"
#include <cmath>

namespace bt {

// ----------------------------------------------------------------------------
ArcShape::ArcShape()
    : m_start(0.0f, 0.0f),
      m_end(0.0f, 0.0f),
      m_control1(0.0f, 0.0f),
      m_control2(0.0f, 0.0f),
      m_color(0, 200, 200, 255),
      m_thickness(3.0f),
      m_num_segments(60),
      m_connection_points_enabled(true),
      m_connection_point_radius(5.0f),
      m_control_point_factor(0.5f)
{
}

// ----------------------------------------------------------------------------
void ArcShape::setPoints(const sf::Vector2f& p_start, const sf::Vector2f& p_end)
{
    m_start = p_start;
    m_end = p_end;
    calculateControlPoints();
}

// ----------------------------------------------------------------------------
void ArcShape::setColor(const sf::Color& p_color)
{
    m_color = p_color;
}

// ----------------------------------------------------------------------------
void ArcShape::setThickness(float p_thickness)
{
    m_thickness = p_thickness;
}

// ----------------------------------------------------------------------------
void ArcShape::setSegments(size_t p_num_segments)
{
    m_num_segments = p_num_segments;
}

// ----------------------------------------------------------------------------
void ArcShape::enableConnectionPoints(bool p_enabled)
{
    m_connection_points_enabled = p_enabled;
}

// ----------------------------------------------------------------------------
void ArcShape::setConnectionPointRadius(float p_radius)
{
    m_connection_point_radius = p_radius;
}

// ----------------------------------------------------------------------------
void ArcShape::setControlPointFactor(float p_factor)
{
    m_control_point_factor = std::max(0.0f, std::min(1.0f, p_factor));
    calculateControlPoints();
}

// ----------------------------------------------------------------------------
void ArcShape::calculateControlPoints()
{
    // Calculate control points for the Bezier curve
    float control_point_offset = (m_end.y - m_start.y) * m_control_point_factor;
    m_control1 = sf::Vector2f(m_start.x, m_start.y + control_point_offset);
    m_control2 = sf::Vector2f(m_end.x, m_end.y - control_point_offset);
}

// ----------------------------------------------------------------------------
void ArcShape::draw(sf::RenderTarget& p_target, sf::RenderStates p_states) const
{
    // Draw the base curve
    drawBaseCurve(p_target, p_states);

    // Draw curve layers for thickness
    drawCurveLayers(p_target, p_states);

    // Draw connection points if enabled
    if (m_connection_points_enabled)
    {
        drawConnectionPoints(p_target, p_states);
    }
}

// ----------------------------------------------------------------------------
void ArcShape::drawBaseCurve(sf::RenderTarget& p_target,
                             sf::RenderStates p_states) const
{
    sf::VertexArray baseCurve(sf::LineStrip, m_num_segments + 1);

    for (size_t i = 0; i <= m_num_segments; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(m_num_segments);

        // Cubic Bezier curve formula
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        sf::Vector2f point = uuu * m_start + 3.0f * uu * t * m_control1 +
                             3.0f * u * tt * m_control2 + ttt * m_end;

        baseCurve[i].position = point;
        baseCurve[i].color =
            sf::Color(m_color.r, m_color.g, m_color.b, 100); // Semi-transparent
    }

    // Draw the base curve
    p_target.draw(baseCurve, p_states);
}

// ----------------------------------------------------------------------------
void ArcShape::drawCurveLayers(sf::RenderTarget& p_target,
                               sf::RenderStates p_states) const
{
    // Draw multiple curve layers with different thicknesses for a smooth effect
    for (float offset = -m_thickness / 2.0f; offset <= m_thickness / 2.0f;
         offset += 0.25f)
    {
        sf::VertexArray curve(sf::LineStrip, m_num_segments + 1);

        for (size_t i = 0; i <= m_num_segments; ++i)
        {
            float t =
                static_cast<float>(i) / static_cast<float>(m_num_segments);

            // Cubic Bezier curve formula
            float u = 1.0f - t;
            float tt = t * t;
            float uu = u * u;
            float uuu = uu * u;
            float ttt = tt * t;

            sf::Vector2f point = uuu * m_start + 3.0f * uu * t * m_control1 +
                                 3.0f * u * tt * m_control2 + ttt * m_end;

            // Add a small perpendicular offset to create thickness
            if ((offset != 0.0f) && (i < m_num_segments))
            {
                // Calculate the next point to determine direction
                float t_next = static_cast<float>(i + 1) /
                               static_cast<float>(m_num_segments);
                float u_next = 1.0f - t_next;
                float tt_next = t_next * t_next;
                float uu_next = u_next * u_next;
                float uuu_next = uu_next * u_next;
                float ttt_next = tt_next * t_next;

                sf::Vector2f next_point =
                    uuu_next * m_start + 3.0f * uu_next * t_next * m_control1 +
                    3.0f * u_next * tt_next * m_control2 + ttt_next * m_end;

                sf::Vector2f direction = next_point - point;
                float length = std::sqrt(direction.x * direction.x +
                                         direction.y * direction.y);

                if (length > 0.0f)
                {
                    direction /= length;
                    // Perpendicular
                    sf::Vector2f perp(-direction.y, direction.x);
                    point += perp * offset;
                }
            }

            // Adjust transparency based on distance from center
            float alpha =
                255.0f * (1.0f - std::abs(offset) / (m_thickness / 2 + 0.5f));
            curve[i].position = point;
            curve[i].color = sf::Color(
                m_color.r, m_color.g, m_color.b, static_cast<sf::Uint8>(alpha));
        }

        // Draw the curve
        p_target.draw(curve, p_states);
    }
}

// ----------------------------------------------------------------------------
void ArcShape::drawConnectionPoints(sf::RenderTarget& p_target,
                                    sf::RenderStates p_states) const
{
    // Draw smooth circles at start and end points
    drawSmoothCircle(
        m_start, m_connection_point_radius, m_color, p_target, p_states);
    drawSmoothCircle(
        m_end, m_connection_point_radius, m_color, p_target, p_states);
}

// ----------------------------------------------------------------------------
void ArcShape::drawSmoothCircle(sf::Vector2f p_position,
                                float p_radius,
                                sf::Color p_color,
                                sf::RenderTarget& p_target,
                                sf::RenderStates p_states,
                                bool p_withBorder) const
{
    // Draw multiple concentric circles with different transparencies for
    // anti-aliasing
    const size_t num_circles = 8;

    if (p_withBorder)
    {
        // Draw white border first
        sf::CircleShape borderCircle;
        borderCircle.setRadius(p_radius + 2.0f);
        borderCircle.setOrigin(p_radius + 2.0f, p_radius + 2.0f);
        borderCircle.setPosition(p_position);
        borderCircle.setFillColor(sf::Color::White);
        p_target.draw(borderCircle, p_states);
    }

    // Draw the main circle with a gradient for anti-aliasing
    for (size_t i = 0; i < num_circles; ++i)
    {
        float current_radius =
            p_radius * (1.0f - 0.1f * static_cast<float>(i) / num_circles);
        float alpha = 255.0f * (1.0f - static_cast<float>(i) / num_circles);

        sf::CircleShape circle;
        circle.setRadius(current_radius);
        circle.setOrigin(current_radius, current_radius);
        circle.setPosition(p_position);
        circle.setFillColor(sf::Color(
            p_color.r, p_color.g, p_color.b, static_cast<sf::Uint8>(alpha)));
        p_target.draw(circle, p_states);
    }
}

} // namespace bt