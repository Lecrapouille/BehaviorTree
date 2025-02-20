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
    //! \brief Constructor with node factory
    //! \param[in] factory Factory used to create action nodes
    explicit TreeBuilder(std::shared_ptr<NodeFactory> factory)
        : m_factory(std::move(factory))
    {}

    // ------------------------------------------------------------------------
    //! \brief Create a behavior tree from YAML file.
    //! \param[in] filename Path to the YAML file.
    //! \return Unique pointer to the created behavior tree.
    // ------------------------------------------------------------------------
    std::unique_ptr<Tree> fromYAML(std::string const& filename);

    // ------------------------------------------------------------------------
    //! \brief Set the factory used to create nodes.
    //! \param[in] factory Pointer to the node factory.
    // ------------------------------------------------------------------------
    static void setFactory(NodeFactory& factory)
    {
        getFactory() = factory;
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

private:
    Node::Ptr parseYAMLNode(YAML::Node const& node);

private:
    std::shared_ptr<NodeFactory> m_factory;
};

} // namespace bt