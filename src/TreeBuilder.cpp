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

#include "BehaviorTree/TreeBuilder.hpp"
#include <yaml-cpp/yaml.h>
#include <tinyxml2.h>

namespace bt {

std::unique_ptr<Tree> TreeBuilder::fromYAML(std::string const& filename)
{
    try
    {
        YAML::Node root = YAML::LoadFile(filename);
        if (!root["behavior_tree"])
        {
            throw std::runtime_error("Missing 'behavior_tree' node in YAML");
        }

        auto tree = std::make_unique<Tree>();
        tree->setRoot(parseYAMLNode(root["behavior_tree"]));
        return tree;
    }
    catch (const YAML::Exception& e)
    {
        throw std::runtime_error("YAML parsing error: " + std::string(e.what()));
    }
}

Node::Ptr TreeBuilder::parseYAMLNode(YAML::Node const& node)
{
    if (!node.IsMap())
    {
        throw std::runtime_error("Invalid node format: must be a map");
    }

    if (!node["type"])
    {
        throw std::runtime_error("Node missing 'type' field");
    }

    std::string type = node["type"].as<std::string>();

    // Composite nodes
    if (type == "sequence")
    {
        auto seq = Node::create<Sequence>();
        if (node["children"] && node["children"].IsSequence())
        {
            for (auto const& child : node["children"])
            {
                seq->addChild(parseYAMLNode(child));
            }
        }
        return seq;
    }
    else if (type == "selector")
    {
        auto sel = Node::create<Selector>();
        if (node["children"] && node["children"].IsSequence())
        {
            for (auto const& child : node["children"])
            {
                sel->addChild(parseYAMLNode(child));
            }
        }
        return sel;
    }
    else if (type == "parallel_sequence")
    {
        size_t success_threshold = node["success_threshold"] ? 
            node["success_threshold"].as<size_t>() : size_t(1);
        size_t failure_threshold = node["failure_threshold"] ? 
            node["failure_threshold"].as<size_t>() : size_t(1);
            
        auto par = Node::create<ParallelSequence>(success_threshold, failure_threshold);
        
        if (node["children"] && node["children"].IsSequence())
        {
            for (const auto& child : node["children"])
            {
                par->addChild(parseYAMLNode(child));
            }
        }
        return par;
    }
    // Decorator nodes
    else if (type == "inverter")
    {
        auto inv = Node::create<Inverter>();
        if (!node["child"])
        {
            throw std::runtime_error("Decorator node missing 'child' field");
        }
        inv->setChild(parseYAMLNode(node["child"]));
        return inv;
    }
    else if (type == "repeater")
    {
        size_t num_repeats = node["num_repeats"] ? 
            node["num_repeats"].as<size_t>() : size_t(0);
        auto rep = Node::create<Repeater>(num_repeats);
        if (!node["child"])
        {
            throw std::runtime_error("Decorator node missing 'child' field");
        }
        rep->setChild(parseYAMLNode(node["child"]));
        return rep;
    }
    // Action nodes
    else if (type == "action")
    {
        if (!node["name"])
        {
            throw std::runtime_error("Action node missing 'name' field");
        }
        std::string action_name = node["name"].as<std::string>();
        auto action = m_factory->createNode(action_name);
        if (!action)
        {
            throw std::runtime_error("Failed to create action node: " + action_name);
        }
        return action;
    }

    throw std::runtime_error("Unknown node type: " + type);
}

} // namespace bt