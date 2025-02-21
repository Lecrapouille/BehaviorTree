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

#include "BehaviorTree/TreeExporter.hpp"

namespace bt {

std::string TreeExporter::toYAML(Tree const& tree)
{
    YAML::Node root;
    root["behavior_tree"] = generateYAMLNode(tree.getRoot());
    return YAML::Dump(root);
}

void TreeExporter::toYAMLFile(Tree const& tree, std::string const& filename)
{
    writeToFile<YAML::Node>(toYAML(tree), filename);
}

std::string TreeExporter::toBTCppXML(Tree const& tree)
{
    std::stringstream xml;
    xml << "<?xml version=\"1.0\" ?>" << std::endl;
    xml << "<root main_tree_to_execute=\"MainTree\">" << std::endl;
    xml << "  <BehaviorTree ID=\"MainTree\">" << std::endl;

    if (tree.getRoot()) {
        generateBTCppXML(tree.getRoot(), xml, 4);
    }

    xml << "  </BehaviorTree>" << std::endl;
    xml << "</root>" << std::endl;
    return xml.str();
}

void TreeExporter::toBTCppXMLFile(Tree const& tree, std::string const& filename)
{
    writeToFile<std::stringstream>(toBTCppXML(tree), filename);
}

YAML::Node TreeExporter::generateYAMLNode(Node::Ptr const& node)
{
    YAML::Node yaml_node;

    if (auto sequence = std::dynamic_pointer_cast<Sequence>(node)) {
        yaml_node["type"] = "sequence";
        YAML::Node children;
        for (auto const& child : sequence->getChildren()) {
            children.push_back(generateYAMLNode(child));
        }
        yaml_node["children"] = children;
    }
    else if (auto selector = std::dynamic_pointer_cast<Selector>(node)) {
        yaml_node["type"] = "selector";
        YAML::Node children;
        for (auto const& child : selector->getChildren()) {
            children.push_back(generateYAMLNode(child));
        }
        yaml_node["children"] = children;
    }
    else if (auto parallel = std::dynamic_pointer_cast<ParallelSequence>(node)) {
        yaml_node["type"] = "parallel";
        YAML::Node children;
        for (auto const& child : parallel->getChildren()) {
            children.push_back(generateYAMLNode(child));
        }
        yaml_node["children"] = children;
    }
    else if (auto inverter = std::dynamic_pointer_cast<Inverter>(node)) {
        yaml_node["type"] = "inverter";
        if (inverter->getChild()) {
            yaml_node["child"] = generateYAMLNode(inverter->getChild());
        }
    }
    else if (auto repeater = std::dynamic_pointer_cast<Repeater>(node)) {
        yaml_node["type"] = "repeater";
        if (repeater->getChild()) {
            yaml_node["child"] = generateYAMLNode(repeater->getChild());
        }
    }
    else if (auto action = std::dynamic_pointer_cast<Action>(node)) {
        yaml_node["type"] = "action";
        yaml_node["name"] = node->name;
    }

    return yaml_node;
}

void TreeExporter::generateBTCppXML(Node::Ptr const& node, std::stringstream& xml, int indent)
{
    std::string spaces(static_cast<size_t>(indent), ' ');

    if (auto sequence = std::dynamic_pointer_cast<Sequence>(node)) {
        xml << spaces << "<Sequence>" << std::endl;
        for (auto const& child : sequence->getChildren()) {
            generateBTCppXML(child, xml, indent + 2);
        }
        xml << spaces << "</Sequence>" << std::endl;
    }
    else if (auto selector = std::dynamic_pointer_cast<Selector>(node)) {
        xml << spaces << "<Fallback>" << std::endl;  // BT.CPP uses Fallback instead of Selector
        for (auto const& child : selector->getChildren()) {
            generateBTCppXML(child, xml, indent + 2);
        }
        xml << spaces << "</Fallback>" << std::endl;
    }
    else if (auto parallel = std::dynamic_pointer_cast<ParallelSequence>(node)) {
        xml << spaces << "<Parallel success_threshold=\"1\" failure_threshold=\"1\">" << std::endl;
        for (auto const& child : parallel->getChildren()) {
            generateBTCppXML(child, xml, indent + 2);
        }
        xml << spaces << "</Parallel>" << std::endl;
    }
    else if (auto inverter = std::dynamic_pointer_cast<Inverter>(node)) {
        xml << spaces << "<Inverter>" << std::endl;
        if (inverter->getChild()) {
            generateBTCppXML(inverter->getChild(), xml, indent + 2);
        }
        xml << spaces << "</Inverter>" << std::endl;
    }
    else if (auto repeater = std::dynamic_pointer_cast<Repeater>(node)) {
        xml << spaces << "<RetryUntilSuccessful num_attempts=\"" << repeater->getLimit() << "\">" << std::endl;
        if (repeater->getChild()) {
            generateBTCppXML(repeater->getChild(), xml, indent + 2);
        }
        xml << spaces << "</RetryUntilSuccessful>" << std::endl;
    }
    else if (auto action = std::dynamic_pointer_cast<Action>(node)) {
        xml << spaces << "<Action ID=\"" << node->name << "\"/>" << std::endl;
    }
}

} // namespace bt