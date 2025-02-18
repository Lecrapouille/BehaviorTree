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

#ifndef BEHAVIOR_TREE_BUILDER_HPP
#  define BEHAVIOR_TREE_BUILDER_HPP

#  include "BehaviorTree/BehaviorTree.hpp"

// ============================================================================
// Forward declarations
// ============================================================================
namespace YAML { class Node; }
namespace tinyxml2 { class XMLElement; class XMLDocument; }

namespace bt {

// ============================================================================
//! \brief Class responsible for building behavior trees from YAML/XML files.
// ============================================================================
class TreeBuilder
{
public:

    // ------------------------------------------------------------------------
    //! \brief Create a behavior tree from YAML file.
    //! \param[in] filename Path to the YAML file.
    //! \return Unique pointer to the created behavior tree.
    // ------------------------------------------------------------------------
    static std::unique_ptr<BehaviorTree> fromYAML(std::string const& filename);

    // ------------------------------------------------------------------------
    //! \brief Create a behavior tree from XML file compatible with
    //! BehaviorTree.CPP https://www.behaviortree.dev
    //! \param[in] filename Path to the XML file.
    //! \return Unique pointer to the created behavior tree.
    // ------------------------------------------------------------------------
    static std::unique_ptr<BehaviorTree> fromXML(std::string const& filename);

    //! \brief Set the factory used to create nodes
    //! \param[in] factory Pointer to the node factory
    static void setFactory(NodeFactory* factory)
    {
        s_factory = factory;
    }

    //! \brief Get the current node factory
    //! \return Pointer to the current node factory
    static NodeFactory* getFactory()
    {
        return s_factory;
    }

private:
    static Node::Ptr parseYAMLNode(YAML::Node const& node);
    static Node::Ptr parseXMLNode(tinyxml2::XMLElement const* element);
    static NodeFactory* s_factory;
};

// Initialize the static member
NodeFactory* TreeBuilder::s_factory = nullptr;

} // namespace bt

#endif