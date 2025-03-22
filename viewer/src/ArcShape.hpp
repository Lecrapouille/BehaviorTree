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

#pragma once

#include <SFML/Graphics.hpp>

namespace bt {

// ****************************************************************************
//! \brief Class for drawing Bezier curves and connection points
// ****************************************************************************
class ArcShape: public sf::Drawable
{
public:

    // ------------------------------------------------------------------------
    //! \brief Default constructor
    // ------------------------------------------------------------------------
    ArcShape();

    // ------------------------------------------------------------------------
    //! \brief Set the start and end points of the arc
    //! \param[in] p_start Start point
    //! \param[in] p_end End point
    // ------------------------------------------------------------------------
    void setPoints(const sf::Vector2f& p_start, const sf::Vector2f& p_end);

    // ------------------------------------------------------------------------
    //! \brief Set the color of the arc
    //! \param[in] p_color Arc color
    // ------------------------------------------------------------------------
    void setColor(const sf::Color& p_color);

    // ------------------------------------------------------------------------
    //! \brief Set the thickness of the arc
    //! \param[in] p_thickness Arc thickness
    // ------------------------------------------------------------------------
    void setThickness(float p_thickness);

    // ------------------------------------------------------------------------
    //! \brief Set the number of segments for the curve
    //! \param[in] p_num_segments Number of segments
    // ------------------------------------------------------------------------
    void setSegments(size_t p_num_segments);

    // ------------------------------------------------------------------------
    //! \brief Enable or disable connection points
    //! \param[in] p_enabled True to enable connection points
    // ------------------------------------------------------------------------
    void enableConnectionPoints(bool p_enabled);

    // ------------------------------------------------------------------------
    //! \brief Set the radius of connection points
    //! \param[in] p_radius Radius of connection points
    // ------------------------------------------------------------------------
    void setConnectionPointRadius(float p_radius);

    // ------------------------------------------------------------------------
    //! \brief Set the control point offset factor
    //! \param[in] p_factor Control point offset factor (0.0 to 1.0)
    // ------------------------------------------------------------------------
    void setControlPointFactor(float p_factor);

private:

    // ------------------------------------------------------------------------
    //! \brief Draw method called by SFML
    //! \param[in] p_target Render target
    //! \param[in] p_states Render states
    // ------------------------------------------------------------------------
    void draw(sf::RenderTarget& p_target,
              sf::RenderStates p_states) const override;

    // ------------------------------------------------------------------------
    //! \brief Draw the base curve
    //! \param[in] p_target Render target
    //! \param[in] p_states Render states
    // ------------------------------------------------------------------------
    void drawBaseCurve(sf::RenderTarget& p_target,
                       sf::RenderStates p_states) const;

    // ------------------------------------------------------------------------
    //! \brief Draw curve layers for thickness
    //! \param[in] p_target Render target
    //! \param[in] p_states Render states
    // ------------------------------------------------------------------------
    void drawCurveLayers(sf::RenderTarget& p_target,
                         sf::RenderStates p_states) const;

    // ------------------------------------------------------------------------
    //! \brief Draw connection points
    //! \param[in] p_target Render target
    //! \param[in] p_states Render states
    // ------------------------------------------------------------------------
    void drawConnectionPoints(sf::RenderTarget& p_target,
                              sf::RenderStates p_states) const;

    // ------------------------------------------------------------------------
    //! \brief Draw a smooth circle
    //! \param[in] p_position Circle position
    //! \param[in] p_radius Circle radius
    //! \param[in] p_color Circle color
    //! \param[in] p_target Render target
    //! \param[in] p_states Render states
    //! \param[in] p_withBorder With border or not
    // ------------------------------------------------------------------------
    void drawSmoothCircle(sf::Vector2f p_position,
                          float p_radius,
                          sf::Color p_color,
                          sf::RenderTarget& p_target,
                          sf::RenderStates p_states,
                          bool p_withBorder = true) const;

    // ------------------------------------------------------------------------
    //! \brief Calculate control points for the Bezier curve
    // ------------------------------------------------------------------------
    void calculateControlPoints();

private:

    //! \brief Start point
    sf::Vector2f m_start;
    //! \brief End point
    sf::Vector2f m_end;
    //! \brief First control point
    sf::Vector2f m_control1;
    //! \brief Second control point
    sf::Vector2f m_control2;
    //! \brief Arc color
    sf::Color m_color;
    //! \brief Arc thickness
    float m_thickness;
    //! \brief Number of segments
    size_t m_num_segments;
    //! \brief Enable connection points
    bool m_connection_points_enabled;
    //! \brief Connection point radius
    float m_connection_point_radius;
    //! \brief Control point offset factor
    float m_control_point_factor;
};

} // namespace bt