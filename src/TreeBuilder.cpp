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

std::unique_ptr<BehaviorTree> TreeBuilder::fromYAML(std::string const& filename)
{
    try {
        YAML::Node root = YAML::LoadFile(filename);
        if (!root["behavior_tree"]) {
            throw std::runtime_error("Missing 'behavior_tree' node in YAML");
        }

        auto tree = std::make_unique<BehaviorTree>();
        tree->setRoot(parseYAMLNode(root["behavior_tree"]));
        return tree;
    }
    catch (const YAML::Exception& e) {
        throw std::runtime_error("YAML parsing error: " + std::string(e.what()));
    }
}

Node::Ptr TreeBuilder::parseYAMLNode(YAML::Node const& node)
{
    if (!node["type"]) {
        throw std::runtime_error("Node missing 'type' field");
    }

    std::string type = node["type"].as<std::string>();

    if (type == "sequence") {
        auto seq = std::make_shared<Sequence>();
        if (node["children"]) {
            for (auto const& child : node["children"]) {
                seq->addChild(parseYAMLNode(child));
            }
        }
        return seq;
    }
    else if (type == "selector") {
        auto sel = std::make_shared<Selector>();
        if (node["children"]) {
            for (auto const& child : node["children"]) {
                sel->addChild(parseYAMLNode(child));
            }
        }
        return sel;
    }
    // Add other node types here...

    throw std::runtime_error("Unknown node type: " + type);
}

std::unique_ptr<BehaviorTree> TreeBuilder::fromXML(std::string const& filename)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Failed to load XML file: " + filename);
    }

    auto root = doc.FirstChildElement("behavior_tree");
    if (!root) {
        throw std::runtime_error("Missing root 'behavior_tree' element");
    }

    auto tree = std::make_unique<BehaviorTree>();
    tree->setRoot(parseXMLNode(root->FirstChildElement()));
    return tree;
}

Node::Ptr TreeBuilder::parseXMLNode(tinyxml2::XMLElement const* element)
{
    if (!element) {
        throw std::runtime_error("Invalid XML element");
    }

    std::string type = element->Name();

    if (type == "Sequence") {
        auto seq = std::make_shared<Sequence>();
        for (auto child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
            seq->addChild(parseXMLNode(child));
        }
        return seq;
    }
    else if (type == "Selector") {
        auto sel = std::make_shared<Selector>();
        for (auto child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
            sel->addChild(parseXMLNode(child));
        }
        return sel;
    }
    // Add other node types here...

    throw std::runtime_error("Unknown node type: " + type);
}

} // namespace bt