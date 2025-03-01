#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>

/**
 * @brief Classe représentant un nœud graphique avec coins arrondis et dégradé
 */
class NodeShape : public sf::Drawable, public sf::Transformable {
public:
    /**
     * @brief Constructeur par défaut
     */
    NodeShape()
    {
        // Initialisation du rectangle de base
        m_background.setFillColor(sf::Color(30, 30, 30));
        m_background.setOutlineThickness(0);

        // Initialisation du dégradé
        for (int i = 0; i < 4; ++i) {
            m_gradientVertices[i].color = sf::Color::Transparent;
        }

        // Initialisation des couleurs par défaut
        m_mainColor = sf::Color(24, 35, 64);                // Bleu foncé (haut)
        m_secondaryColor = sf::Color(16, 24, 45);           // Bleu très foncé (bas)
        m_borderColor = sf::Color(0, 255, 255, 200);        // Cyan semi-transparent

        // Ne pas initialiser le texte ici, il sera créé dans setText()

        updateGeometry();
    }

    /**
     * @brief Définit le rayon des coins arrondis
     *
     * @param[in] radius Rayon des coins
     */
    void setCornerRadius(float radius)
    {
        m_radius = radius;
        updateGeometry();
    }

    /**
     * @brief Définit le texte du nœud
     *
     * @param[in] text Texte à afficher
     * @param[in] font Police à utiliser
     * @param[in] charSize Taille des caractères
     */
    void setText(const std::string& text, const sf::Font& font, unsigned int charSize = 24)
    {
        // Créer un nouveau texte plutôt que de modifier l'existant
        m_text = sf::Text();
        m_text.setString(text);
        m_text.setFont(font);
        m_text.setCharacterSize(charSize);

        // Forcer la couleur blanche et désactiver tout contour
        m_text.setFillColor(sf::Color::White);
        m_text.setOutlineThickness(0);
        m_text.setOutlineColor(sf::Color::Transparent);
        m_text.setStyle(sf::Text::Regular);

        updateGeometry();
    }

    /**
     * @brief Définit l'icône du nœud
     *
     * @param[in] texture Texture de l'icône
     * @param[in] scale Échelle de l'icône
     */
    void setIcon(const sf::Texture& texture, float scale = 1.0f)
    {
        m_icon.setTexture(texture);
        m_icon.setScale(scale, scale);
        m_icon.setColor(sf::Color::White);  // S'assurer que l'icône est blanche
        updateGeometry();
    }

    /**
     * @brief Définit les couleurs du dégradé
     *
     * @param[in] mainColor Couleur principale (haut)
     * @param[in] secondaryColor Couleur secondaire (bas)
     * @param[in] borderColor Couleur de la bordure
     */
    void setColors(const sf::Color& mainColor, const sf::Color& secondaryColor,
                  const sf::Color& borderColor = sf::Color(0, 255, 255, 200))
    {
        // Couleurs du dégradé vertical et de la bordure
        m_mainColor = mainColor;
        m_secondaryColor = secondaryColor;
        m_borderColor = borderColor;

        // Mise à jour des couleurs du dégradé
        m_gradientVertices[0].color = mainColor;
        m_gradientVertices[1].color = mainColor;
        m_gradientVertices[2].color = secondaryColor;
        m_gradientVertices[3].color = secondaryColor;

        // Mise à jour des couleurs du rectangle arrondi
        updateRoundedRectangle();
        updateBorder();
    }

    /**
     * @brief Définit les marges internes
     *
     * @param[in] horizontal Marge horizontale
     * @param[in] vertical Marge verticale
     */
    void setPadding(float horizontal, float vertical)
    {
        m_padding = {horizontal, vertical};
        updateGeometry();
    }

    /**
     * @brief Récupère les dimensions du nœud
     *
     * @return Dimensions (largeur, hauteur)
     */
    sf::Vector2f getDimensions() const
    {
        return m_currentSize;
    }

private:
    /**
     * @brief Méthode de dessin appelée par SFML
     *
     * @param[in] target Cible de rendu
     * @param[in] states États de rendu
     */
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        states.transform *= getTransform();

        // Dessin du rectangle arrondi avec dégradé
        target.draw(m_roundedRectangle, states);

        // Dessin de la bordure cyan
        target.draw(m_border, states);

        // Dessin de l'icône si disponible
        if (m_icon.getTexture()) {
            target.draw(m_icon, states);
        }

        // Dessin du texte uniquement s'il a une police valide
        if (m_text.getFont()) {
            // Ne pas modifier le texte dans la méthode const draw()
            // Les propriétés doivent être définies avant l'appel à draw()
            target.draw(m_text, states);
        }
    }

    /**
     * @brief Met à jour la géométrie du nœud
     */
    void updateGeometry()
    {
        // Dimensions par défaut si pas de texte
        float width = 100.0f;
        float height = 40.0f;

        // Calcul des dimensions si le texte a une police valide
        if (m_text.getFont()) {
            sf::FloatRect textBounds = m_text.getLocalBounds();
            sf::FloatRect iconBounds;

            // Prise en compte de l'icône uniquement si une texture est définie
            float iconWidth = 0.0f;
            if (m_icon.getTexture()) {
                iconBounds = m_icon.getGlobalBounds();
                iconWidth = iconBounds.width + 15.0f; // Espacement entre l'icône et le texte
            }

            // Calcul de la largeur et hauteur avec espacement approprié
            width = textBounds.width + iconWidth + m_padding.x * 2;
            height = std::max(textBounds.height, iconBounds.height) + m_padding.y * 2;

            // Dimensions minimales
            width = std::max(width, 100.0f);
            height = std::max(height, 40.0f);

            // Centrage vertical du texte et positionnement après l'icône
            m_text.setOrigin(0, textBounds.top + textBounds.height/2.0f);
            float textX = m_padding.x + iconWidth;
            float textY = height / 2.0f;
            m_text.setPosition(textX, textY);

            // Positionnement de l'icône si disponible
            if (m_icon.getTexture()) {
                m_icon.setOrigin(0, iconBounds.height / 2.0f);
                m_icon.setPosition(m_padding.x, height / 2.0f);
            }
        }

        m_currentSize = {width, height};

        // Mise à jour du rectangle arrondi et de la bordure
        updateRoundedRectangle();
        updateBorder();
    }

    /**
     * @brief Met à jour le rectangle arrondi avec dégradé
     */
    void updateRoundedRectangle()
    {
        float width = m_currentSize.x;
        float height = m_currentSize.y;
        float radius = m_radius;

        // Limiter le rayon à la moitié de la plus petite dimension
        radius = std::min(radius, std::min(width/2.0f, height/2.0f));

        // Réinitialiser le rectangle arrondi
        m_roundedRectangle.clear();

        // Ajouter le point central pour le triangle fan
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(width/2.0f, height/2.0f),
                                            interpolateColor(m_mainColor, m_secondaryColor, 0.5f)));

        // Nombre de segments pour les coins arrondis (plus élevé = plus lisse)
        const int cornerSegments = 8;

        // Créer les vertices pour le rectangle arrondi avec dégradé

        // Coin supérieur gauche
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = M_PIf + (M_PIf/2.0f) * i / cornerSegments;
            float x = radius + radius * cosf(angle);
            float y = radius + radius * sinf(angle);
            sf::Vertex vertex(sf::Vector2f(x, y), m_mainColor);
            m_roundedRectangle.append(vertex);
        }

        // Bord supérieur
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(radius, 0), m_mainColor));
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(width - radius, 0), m_mainColor));

        // Coin supérieur droit
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = 3.0f * M_PIf / 2.0f + (M_PIf/2.0f) * i / cornerSegments;
            float x = width - radius + radius * cosf(angle);
            float y = radius + radius * sinf(angle);
            sf::Vertex vertex(sf::Vector2f(x, y), m_mainColor);
            m_roundedRectangle.append(vertex);
        }

        // Bord droit
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(width, radius), m_mainColor));
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(width, height - radius),
                                            interpolateColor(m_mainColor, m_secondaryColor, 0.7f)));

        // Coin inférieur droit
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = 0 + (M_PIf/2.0f) * i / cornerSegments;
            float x = width - radius + radius * cosf(angle);
            float y = height - radius + radius * sinf(angle);
            sf::Vertex vertex(sf::Vector2f(x, y), m_secondaryColor);
            m_roundedRectangle.append(vertex);
        }

        // Bord inférieur
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(width - radius, height), m_secondaryColor));
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(radius, height), m_secondaryColor));

        // Coin inférieur gauche
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = M_PIf/2.0f + (M_PIf/2.0f) * i / cornerSegments;
            float x = radius + radius * cosf(angle);
            float y = height - radius + radius * sinf(angle);
            sf::Vertex vertex(sf::Vector2f(x, y), m_secondaryColor);
            m_roundedRectangle.append(vertex);
        }

        // Bord gauche
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(0, height - radius),
                                            interpolateColor(m_mainColor, m_secondaryColor, 0.7f)));
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(0, radius), m_mainColor));

        // Fermer le contour
        float angle = M_PIf + (M_PIf/2.0f) * 0 / cornerSegments;
        float x = radius + radius * cosf(angle);
        float y = radius + radius * sinf(angle);
        m_roundedRectangle.append(sf::Vertex(sf::Vector2f(x, y), m_mainColor));
    }

    /**
     * @brief Met à jour la bordure cyan
     */
    void updateBorder()
    {
        float width = m_currentSize.x;
        float height = m_currentSize.y;
        float radius = m_radius;

        // Limiter le rayon à la moitié de la plus petite dimension
        radius = std::min(radius, std::min(width/2.0f, height/2.0f));

        // Réinitialiser la bordure
        m_border.clear();
        m_border.setPrimitiveType(sf::LineStrip);

        // Nombre de segments pour les coins arrondis
        const int cornerSegments = 16;  // Plus de segments pour une bordure plus lisse

        // Coin supérieur gauche
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = M_PIf + (M_PIf/2.0f) * i / cornerSegments;
            float x = radius + radius * cosf(angle);
            float y = radius + radius * sinf(angle);
            m_border.append(sf::Vertex(sf::Vector2f(x, y), m_borderColor));
        }

        // Bord supérieur
        m_border.append(sf::Vertex(sf::Vector2f(width - radius, 0), m_borderColor));

        // Coin supérieur droit
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = 3.0f * M_PIf / 2.0f + (M_PIf/2.0f) * i / cornerSegments;
            float x = width - radius + radius * cosf(angle);
            float y = radius + radius * sinf(angle);
            m_border.append(sf::Vertex(sf::Vector2f(x, y), m_borderColor));
        }

        // Bord droit
        m_border.append(sf::Vertex(sf::Vector2f(width, height - radius), m_borderColor));

        // Coin inférieur droit
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = 0 + (M_PIf/2.0f) * i / cornerSegments;
            float x = width - radius + radius * cosf(angle);
            float y = height - radius + radius * sinf(angle);
            m_border.append(sf::Vertex(sf::Vector2f(x, y), m_borderColor));
        }

        // Bord inférieur
        m_border.append(sf::Vertex(sf::Vector2f(radius, height), m_borderColor));

        // Coin inférieur gauche
        for (int i = 0; i <= cornerSegments; ++i) {
            float angle = M_PIf/2.0f + (M_PIf/2.0f) * i / cornerSegments;
            float x = radius + radius * cosf(angle);
            float y = height - radius + radius * sinf(angle);
            m_border.append(sf::Vertex(sf::Vector2f(x, y), m_borderColor));
        }

        // Bord gauche et fermeture
        m_border.append(sf::Vertex(sf::Vector2f(0, radius), m_borderColor));

        // Fermer la bordure en revenant au point de départ
        float closeAngle = M_PIf + (M_PIf/2.0f) * 0 / cornerSegments;
        float closeX = radius + radius * cosf(closeAngle);
        float closeY = radius + radius * sinf(closeAngle);
        m_border.append(sf::Vertex(sf::Vector2f(closeX, closeY), m_borderColor));
    }

    /**
     * @brief Interpole entre deux couleurs
     *
     * @param[in] color1 Première couleur
     * @param[in] color2 Deuxième couleur
     * @param[in] factor Facteur d'interpolation (0.0 à 1.0)
     * @return Couleur interpolée
     */
    sf::Color interpolateColor(const sf::Color& color1, const sf::Color& color2, float factor) const
    {
        return sf::Color(
            color1.r + (color2.r - color1.r) * factor,
            color1.g + (color2.g - color1.g) * factor,
            color1.b + (color2.b - color1.b) * factor,
            color1.a + (color2.a - color1.a) * factor
        );
    }

private:
    sf::Text m_text;
    sf::Sprite m_icon;
    sf::RectangleShape m_background;
    sf::Vertex m_gradientVertices[4];
    sf::VertexArray m_roundedRectangle{sf::TriangleFan};
    sf::VertexArray m_border{sf::LineStrip};
    sf::Vector2f m_padding{15.0f, 12.0f};  // Padding ajusté pour correspondre à l'image
    float m_radius{8.0f};                  // Rayon ajusté pour correspondre à l'image
    sf::Vector2f m_currentSize{100.0f, 40.0f};
    sf::Color m_mainColor;                 // Couleur principale (haut)
    sf::Color m_secondaryColor;            // Couleur secondaire (bas)
    sf::Color m_borderColor;               // Couleur de la bordure
};
