//*****************************************************************************
// A C++ behavior tree lib https://github.com/Lecrapouille/BehaviorTree
//
// MIT License
//
// Copyright (c) 2024 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
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

#  include "BehaviorTree/BehaviorTree.hpp"

// ****************************************************************************
// Forward declarations
// ****************************************************************************
namespace YAML { class Node; }

namespace bt {

// ****************************************************************************
//! \brief Class responsible for building behavior trees from YAML/XML files.
// ****************************************************************************
class TreeBuilder
{
public:
    // ------------------------------------------------------------------------
    //! \brief Constructor with node factory.
    //! \param[in] factory Factory used to create action nodes.
    // ------------------------------------------------------------------------
    explicit TreeBuilder(std::shared_ptr<NodeFactory> factory)
        : m_factory(std::move(factory))
    {}

    // ------------------------------------------------------------------------
    //! \brief Default constructor. You shall set a factory before using the builder.
    // ------------------------------------------------------------------------
    TreeBuilder() = default;

    // ------------------------------------------------------------------------
    //! \brief Create a behavior tree from YAML file.
    //! \param[in] filename Path to the YAML file.
    //! \return Unique pointer to the created behavior tree.
    // ------------------------------------------------------------------------
    std::unique_ptr<Tree> fromYAML(std::string const& filename);

    // ------------------------------------------------------------------------
    //! \brief Create a behavior tree from YAML text string.
    //! \param[in] yaml_text String containing the YAML definition.
    //! \return Unique pointer to the created behavior tree.
    // ------------------------------------------------------------------------
    std::unique_ptr<Tree> fromText(std::string const& yaml_text);

    // ------------------------------------------------------------------------
    //! \brief Set the factory used to create nodes.
    //! \param[in] factory Pointer to the node factory.
    // ------------------------------------------------------------------------
    static void setFactory(NodeFactory& factory)
    {
        getFactory() = factory;
    }

    // ------------------------------------------------------------------------
    //! \brief Create and set a factory of type T (default is NodeFactory).
    //! \tparam T Type of the factory (must inherit from NodeFactory).
    //! \param[in] blackboard Optional blackboard to pass to the factory.
    // ------------------------------------------------------------------------
    template<typename T = NodeFactory>
    static void setFactory(Blackboard::Ptr blackboard)
    {
        static_assert(std::is_base_of<NodeFactory, T>::value,
                     "T must inherit from NodeFactory");
        getFactory() = std::make_shared<T>(blackboard);
    }

    // ------------------------------------------------------------------------
    //! \brief Create and set a factory of type T (default is NodeFactory).
    //! \tparam T Type of the factory (must inherit from NodeFactory).
    // ------------------------------------------------------------------------
    template<typename T = NodeFactory>
    static void setFactory()
    {
        static_assert(std::is_base_of<NodeFactory, T>::value,
                     "T must inherit from NodeFactory");
        getFactory() = std::make_shared<T>();
    }

    // ------------------------------------------------------------------------
    //! \brief Get the current node factory.
    //! \return Pointer to the current node factory.
    // ------------------------------------------------------------------------
    static NodeFactory& getFactory()
    {
        static NodeFactory s_factory;
        return s_factory;
    }

    // ------------------------------------------------------------------------
    //! \brief Export the behavior tree to Mermaid format.
    //! \param[in] tree The behavior tree to export.
    //! \return String containing the Mermaid diagram definition.
    // ------------------------------------------------------------------------
    static std::string toMermaid(Tree const& tree);

private:
    Node::Ptr parseYAMLNode(YAML::Node const& node);

    // ------------------------------------------------------------------------
    //! \brief Helper method to recursively generate Mermaid nodes.
    //! \param[in] node Current node being processed.
    //! \param[in] parent_id ID of the parent node.
    //! \param[inout] counter Running counter for generating unique node IDs.
    //! \param[inout] result String containing the Mermaid diagram content.
    // ------------------------------------------------------------------------
    static void generateMermaidNode(Node const* node, size_t parent_id,
                                  size_t& counter, std::string& result);

private:
    std::shared_ptr<NodeFactory> m_factory;
};

} // namespace bt