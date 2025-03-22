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

// ****************************************************************************
// Forward declarations
// ****************************************************************************
namespace YAML {
class Node;
}

namespace bt {

// ============================================================================
//! \brief Class responsible for exporting behavior trees to various formats
// ============================================================================
class TreeExporter
{
public:

    // ------------------------------------------------------------------------
    //! \brief Export behavior tree to our YAML format.
    //! \param[in] tree The behavior tree to export.
    //! \return YAML string representation.
    // ------------------------------------------------------------------------
    static std::string toYAML(Tree const& tree);

    // ------------------------------------------------------------------------
    //! \brief Export behavior tree to our YAML file.
    //! \param[in] tree The behavior tree to export.
    //! \param[in] filename Path to save the YAML file.
    //! \throw std::runtime_error if file cannot be written.
    //! \return True if file was written successfully, false otherwise.
    // ------------------------------------------------------------------------
    static bool toYAMLFile(Tree const& tree, std::string const& filename);

    // ------------------------------------------------------------------------
    //! \brief Export behavior tree to BehaviorTree.CPP XML format.
    //! \param[in] tree The behavior tree to export.
    //! \return XML string representation.
    // ------------------------------------------------------------------------
    static std::string toBTCppXML(Tree const& tree);

    // ------------------------------------------------------------------------
    //! \brief Export behavior tree to BehaviorTree.CPP XML file.
    //! \param[in] tree The behavior tree to export.
    //! \param[in] filename Path to save the XML file.
    //! \return True if file was written successfully, false otherwise.
    // ------------------------------------------------------------------------
    static bool toBTCppXMLFile(Tree const& tree, std::string const& filename);

    // ------------------------------------------------------------------------
    //! \brief Export the behavior tree to Mermaid format.
    //! \param[in] tree The behavior tree to export.
    //! \return String containing the Mermaid diagram definition.
    // ------------------------------------------------------------------------
    static std::string toMermaid(Tree const& tree);

private:

    static void generateMermaidNode(Node const* node,
                                    size_t parent_id,
                                    size_t& counter,
                                    std::string& result);
    static YAML::Node generateYAMLNode(Node const* node);
    static void
    generateBTCppXML(Node const* node, std::stringstream& xml, int indent);
};

} // namespace bt