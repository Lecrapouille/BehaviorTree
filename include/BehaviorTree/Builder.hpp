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

#include "BehaviorTree/BehaviorTree.hpp"

namespace YAML {
class Node;
}

namespace bt {

// ****************************************************************************
//! \brief Class responsible for building behavior trees from YAML/XML files.
// ****************************************************************************
class Builder
{
public:

    // ------------------------------------------------------------------------
    //! \brief Create a behavior tree from YAML file.
    //! \param[in] p_file_path Path to the YAML file.
    //! \return Unique pointer to the created behavior tree.
    // ------------------------------------------------------------------------
    static std::unique_ptr<Tree> fromFile(NodeFactory const& p_factory,
                                          std::string const& p_file_path);

    // ------------------------------------------------------------------------
    //! \brief Create a behavior tree from YAML text string.
    //! \param[in] p_yaml_text String containing the YAML definition.
    //! \return Unique pointer to the created behavior tree.
    // ------------------------------------------------------------------------
    static std::unique_ptr<Tree> fromText(NodeFactory const& p_factory,
                                          std::string const& p_yaml_text);

private:

    static Node::Ptr parseYAMLNode(NodeFactory const& p_factory,
                                   YAML::Node const& node);
};

} // namespace bt