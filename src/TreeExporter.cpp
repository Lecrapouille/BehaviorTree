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
#include <yaml-cpp/yaml.h>
#include <sstream>

namespace bt {

std::string TreeExporter::toYAML(Tree const& tree)
{
    YAML::Node root;
    
    // Get the type of the root node
    auto root_node = tree.getRoot();
    if (!root_node)
    {
        throw std::runtime_error("Tree has no root node");
    }

    // Generate the root node directly
    root["behavior_tree"] = generateYAMLNode(root_node);
    
    return YAML::Dump(root);
}

bool TreeExporter::toYAMLFile(Tree const& tree, std::string const& filename)
{
    return writeToFile<YAML::Node>(toYAML(tree), filename);
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

bool TreeExporter::toBTCppXMLFile(Tree const& tree, std::string const& filename)
{
    return writeToFile<std::stringstream>(toBTCppXML(tree), filename);
}

YAML::Node TreeExporter::generateYAMLNode(Node::Ptr const& node)
{
    YAML::Node yaml_node;

    if (auto sequence = std::dynamic_pointer_cast<Sequence>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "sequence")
        {
            content["name"] = node->name;
        }
        YAML::Node children;
        for (auto const& child : sequence->getChildren())
        {
            children.push_back(generateYAMLNode(child));
        }
        content["children"] = children;
        yaml_node["Sequence"] = content;
    }
    else if (auto selector = std::dynamic_pointer_cast<Selector>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "selector")
        {
            content["name"] = node->name;
        }
        YAML::Node children;
        for (auto const& child : selector->getChildren())
        {
            children.push_back(generateYAMLNode(child));
        }
        content["children"] = children;
        yaml_node["Selector"] = content;
    }
    else if (auto parallel = std::dynamic_pointer_cast<Parallel>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "parallel")
        {
            content["name"] = node->name;
        }
        content["success_threshold"] = parallel->getMinSuccess();
        content["failure_threshold"] = parallel->getMinFail();
        YAML::Node children;
        for (auto const& child : parallel->getChildren())
        {
            children.push_back(generateYAMLNode(child));
        }
        content["children"] = children;
        yaml_node["Parallel"] = content;
    }
    else if (auto parallel_all = std::dynamic_pointer_cast<ParallelAll>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "parallel")
        {
            content["name"] = node->name;
        }
        content["success_on_all"] = parallel_all->getSuccessOnAll();
        content["fail_on_all"] = parallel_all->getFailOnAll();
        YAML::Node children;
        for (auto const& child : parallel_all->getChildren())
        {
            children.push_back(generateYAMLNode(child));
        }
        content["children"] = children;
        yaml_node["Parallel"] = content;
    }
    else if (auto inverter = std::dynamic_pointer_cast<Inverter>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "inverter")
        {
            content["name"] = node->name;
        }
        if (inverter->getChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(inverter->getChild()));
            content["child"] = child_seq;
        }
        yaml_node["Inverter"] = content;
    }
    else if (auto retry = std::dynamic_pointer_cast<Retry>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "retry")
        {
            content["name"] = node->name;
        }
        content["attempts"] = retry->getAttempts();
        if (retry->getChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(retry->getChild()));
            content["child"] = child_seq;
        }
        yaml_node["Retry"] = content;
    }
    else if (auto repeat = std::dynamic_pointer_cast<Repeat>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "repeat")
        {
            content["name"] = node->name;
        }
        content["times"] = repeat->getRepetitions();
        if (repeat->getChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(repeat->getChild()));
            content["child"] = child_seq;
        }
        yaml_node["Repeat"] = content;
    }
    else if (auto until_success = std::dynamic_pointer_cast<UntilSuccess>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "repeat_until_success")
        {
            content["name"] = node->name;
        }
        if (until_success->getChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(until_success->getChild()));
            content["child"] = child_seq;
        }
        yaml_node["RepeatUntilSuccess"] = content;
    }
    else if (auto until_failure = std::dynamic_pointer_cast<UntilFailure>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "repeat_until_failure")
        {
            content["name"] = node->name;
        }
        if (until_failure->getChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(until_failure->getChild()));
            content["child"] = child_seq;
        }
        yaml_node["RepeatUntilFailure"] = content;
    }
    else if (auto condition = std::dynamic_pointer_cast<Condition>(node))
    {
        YAML::Node content;
        content["name"] = node->name;
        yaml_node["Condition"] = content;
    }
    else if (auto action = std::dynamic_pointer_cast<Action>(node))
    {
        YAML::Node content;
        content["name"] = node->name;
        yaml_node["Action"] = content;
    }
    else
    {
        throw std::runtime_error("Unknown node type");
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
    else if (auto parallel = std::dynamic_pointer_cast<Parallel>(node)) {
        xml << spaces << "<Parallel success_threshold=\"" << parallel->getMinSuccess()
            << "\" failure_threshold=\"" << parallel->getMinFail() << "\">" << std::endl;
        for (auto const& child : parallel->getChildren()) {
            generateBTCppXML(child, xml, indent + 2);
        }
        xml << spaces << "</Parallel>" << std::endl;
    }
    else if (auto parallel_all = std::dynamic_pointer_cast<ParallelAll>(node)) {
        xml << spaces << "<Parallel success_threshold=\"" 
            << (parallel_all->getSuccessOnAll() ? parallel_all->getChildren().size() : 1)
            << "\" failure_threshold=\""
            << (parallel_all->getFailOnAll() ? parallel_all->getChildren().size() : 1)
            << "\">" << std::endl;
        for (auto const& child : parallel_all->getChildren()) {
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
    else if (auto repeat = std::dynamic_pointer_cast<Repeat>(node)) {
        xml << spaces << "<RetryUntilSuccessful num_attempts=\"" << repeat->getRepetitions() << "\">" << std::endl;
        if (repeat->getChild()) {
            generateBTCppXML(repeat->getChild(), xml, indent + 2);
        }
        xml << spaces << "</RetryUntilSuccessful>" << std::endl;
    }
    else if (auto action = std::dynamic_pointer_cast<Action>(node)) {
        xml << spaces << "<Action ID=\"" << node->name << "\"/>" << std::endl;
    }
}

} // namespace bt