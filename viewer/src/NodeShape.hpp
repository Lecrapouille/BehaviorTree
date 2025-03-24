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

#include <SFML/Graphics.hpp>

#pragma once

namespace bt {

// ****************************************************************************
//! \brief Class representing a node with rounded corners and gradient
// ****************************************************************************
class NodeShape: public sf::Drawable, public sf::Transformable
{
public:

    // ------------------------------------------------------------------------
    //! \brief Default constructor.
    // ------------------------------------------------------------------------
    NodeShape();

    // ------------------------------------------------------------------------
    //! \brief Set the corner radius.
    //! \param[in] p_radius Corner radius.
    // ------------------------------------------------------------------------
    void setCornerRadius(float p_radius);

    // ------------------------------------------------------------------------
    //! \brief Set the text of the node.
    //! \param[in] p_text Text to display.
    //! \param[in] p_font Font to use.
    //! \param[in] p_charSize Character size.
    // ------------------------------------------------------------------------
    void setText(const std::string& p_text,
                 const sf::Font& p_font,
                 unsigned int p_charSize = 24);

    // ------------------------------------------------------------------------
    //! \brief Set the icon of the node.
    //! \param[in] p_texture Icon texture.
    //! \param[in] p_scale Icon scale.
    // ------------------------------------------------------------------------
    void setIcon(const sf::Texture& p_texture, float p_scale = 1.0f);

    // ------------------------------------------------------------------------
    //! \brief Set the colors of the gradient.
    //! \param[in] p_mainColor Main color (top).
    //! \param[in] p_secondaryColor Secondary color (bottom).
    //! \param[in] p_borderColor Border color.
    // ------------------------------------------------------------------------
    void
    setColors(const sf::Color& p_mainColor,
              const sf::Color& p_secondaryColor,
              const sf::Color& p_borderColor = sf::Color(0, 255, 255, 200));

    // ------------------------------------------------------------------------
    //! \brief Enable or disable text smoothing.
    //! \param[in] p_smooth True to enable text smoothing, false otherwise.
    // ------------------------------------------------------------------------
    void setTextSmoothing(bool p_smooth);

    // ------------------------------------------------------------------------
    //! \brief Set the internal margins.
    //! \param[in] p_horizontal Horizontal margin.
    //! \param[in] p_vertical Vertical margin.
    // ------------------------------------------------------------------------
    void setPadding(float p_horizontal, float p_vertical);

    // ------------------------------------------------------------------------
    //! \brief Get the dimensions of the node.
    //! \return Dimensions (width, height).
    // ------------------------------------------------------------------------
    sf::Vector2f getDimensions() const
    {
        return m_currentSize;
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Draw method called by SFML.
    //! \param[in] target Render target.
    //! \param[in] p_states Render states.
    // ------------------------------------------------------------------------
    void draw(sf::RenderTarget& p_target,
              sf::RenderStates p_states) const override;

    // ------------------------------------------------------------------------
    //! \brief Update the geometry of the node.
    // ------------------------------------------------------------------------
    void updateGeometry();

    // ------------------------------------------------------------------------
    //! \brief Update the rounded rectangle with gradient.
    // ------------------------------------------------------------------------
    void updateRoundedRectangle();

    // ------------------------------------------------------------------------
    //! \brief Update the cyan border.
    // ------------------------------------------------------------------------
    void updateBorder();

    // ------------------------------------------------------------------------
    //! \brief Interpolate between two colors.
    //! \param[in] p_color1 First color.
    //! \param[in] p_color2 Second color.
    //! \param[in] p_factor Interpolation factor (0.0 to 1.0).
    //! \return Interpolated color.
    // ------------------------------------------------------------------------
    sf::Color interpolateColor(const sf::Color& p_color1,
                               const sf::Color& p_color2,
                               float p_factor) const;

private:

    //! \brief Text to display.
    sf::Text m_text;
    //! \brief Icon to display.
    sf::Sprite m_icon;
    //! \brief Background of the node.
    sf::RectangleShape m_background;
    //! \brief Gradient vertices.
    sf::Vertex m_gradient_vertices[4];
    //! \brief Rounded rectangle.
    sf::VertexArray m_rounded_rectangle{sf::TriangleFan};
    //! \brief Border.
    sf::VertexArray m_border{sf::TriangleStrip};
    //! \brief Padding.
    sf::Vector2f m_padding{15.0f, 12.0f};
    //! \brief Radius.
    float m_radius{8.0f};
    //! \brief Current size.
    sf::Vector2f m_currentSize{100.0f, 40.0f};
    //! \brief Main color.
    sf::Color m_mainColor;
    //! \brief Secondary color.
    sf::Color m_secondaryColor;
    //! \brief Border color.
    sf::Color m_borderColor;
    //! \brief Text smoothing.
    bool m_textSmoothing{true};
};

} // namespace bt
