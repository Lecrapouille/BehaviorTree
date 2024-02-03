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

#include "BehaviorTree/BehaviorTree.hpp"
#include "tinyxml2/tinyxml2.h"
#include <sstream>
#include <unordered_map>

namespace bt {

//------------------------------------------------------------------------------
std::string import(bt::BehaviorTree& tree, std::string const& filename)
{
    enum class NodeType { UNDEFINED, ACTION, CONDITION, CONTROL, DECORATOR, SUBTREE };
    std::stringstream error;

    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason was '"
              << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    tinyxml2::XMLDocument doc;
    doc.LoadFile(filename.c_str());

    tinyxml2::XMLElement* root = doc->RootElement();
    if (!xml_root->Attribute("BTCPP_format"))
    {
        error << "Error: The first tag of the XML (<root>) should contain"
              << " the attribute [BTCPP_format=\"4\"]" << std::endl;
        return error.str();
    }

    // Collect the names of all nodes registered with the behavior tree factory
    //std::unordered_map<std::string, NodeType> registered_nodes;
    //for (const auto& it : factory.manifests())
    //{
    //    registered_nodes.insert({it.first, it.second.type});
    //}

    // Register each BehaviorTree within the XML
    for (auto node = xml_root->FirstChildElement("BehaviorTree");
        node != nullptr; node = node->NextSiblingElement("BehaviorTree"))
    {
        std::string tree_name;
        if (node->Attribute("ID"))
        {
            tree_name = bt_node->Attribute("ID");
        }
        else
        {
            tree_name = "BehaviorTree_" + std::to_string(suffix_count++);
        }

        tree_roots[tree_name] = bt_node;
    }
}

QQQQQQQQQQ
void BT::XMLParser::PImpl::recursivelyCreateSubtree(

} // namespace bt