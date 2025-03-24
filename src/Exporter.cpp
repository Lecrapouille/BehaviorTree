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

#include "BehaviorTree/Exporter.hpp"

#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace bt {

// ----------------------------------------------------------------------------
template <typename T>
static bool writeToFile(std::string const& content, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }
    file << content;
    return true;
}

// ----------------------------------------------------------------------------
std::string Exporter::toYAML(Tree const& tree)
{
    if (!tree.hasRoot())
        return {};

    YAML::Node root;
    root["behavior_tree"] = generateYAMLNode(&(tree.getRoot()));
    return YAML::Dump(root);
}

// ----------------------------------------------------------------------------
bool Exporter::toYAMLFile(Tree const& tree, std::string const& filename)
{
    return writeToFile<YAML::Node>(toYAML(tree), filename);
}

// ----------------------------------------------------------------------------
std::string Exporter::toBTCppXML(Tree const& tree)
{
    std::stringstream xml;
    xml << "<?xml version=\"1.0\" ?>" << std::endl;
    xml << "<root main_tree_to_execute=\"MainTree\">" << std::endl;
    xml << "  <BehaviorTree ID=\"MainTree\">" << std::endl;

    if (tree.hasRoot())
    {
        generateBTCppXML(&(tree.getRoot()), xml, 4);
    }

    xml << "  </BehaviorTree>" << std::endl;
    xml << "</root>" << std::endl;
    return xml.str();
}

// ----------------------------------------------------------------------------
bool Exporter::toBTCppXMLFile(Tree const& tree, std::string const& filename)
{
    return writeToFile<std::stringstream>(toBTCppXML(tree), filename);
}

// ----------------------------------------------------------------------------
YAML::Node Exporter::generateYAMLNode(Node const* node)
{
    YAML::Node yaml_node;

    if (auto sequence = dynamic_cast<Sequence const*>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "sequence")
        {
            content["name"] = node->name;
        }

        YAML::Node children;
        for (auto const& child : sequence->getChildren())
        {
            children.push_back(generateYAMLNode(child.get()));
        }
        content["children"] = children;
        yaml_node["Sequence"] = content;
    }
    else if (auto selector = dynamic_cast<Selector const*>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "selector")
        {
            content["name"] = node->name;
        }

        YAML::Node children;
        for (auto const& child : selector->getChildren())
        {
            children.push_back(generateYAMLNode(child.get()));
        }
        content["children"] = children;
        yaml_node["Selector"] = content;
    }
    else if (auto parallel = dynamic_cast<Parallel const*>(node))
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
            children.push_back(generateYAMLNode(child.get()));
        }
        content["children"] = children;
        yaml_node["Parallel"] = content;
    }
    else if (auto parallel_all = dynamic_cast<ParallelAll const*>(node))
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
            children.push_back(generateYAMLNode(child.get()));
        }
        content["children"] = children;
        yaml_node["Parallel"] = content;
    }
    else if (auto inverter = dynamic_cast<Inverter const*>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "inverter")
        {
            content["name"] = node->name;
        }
        if (inverter->hasChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(&(inverter->getChild())));
            content["child"] = child_seq;
        }
        yaml_node["Inverter"] = content;
    }
    else if (auto retry = dynamic_cast<Retry const*>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "retry")
        {
            content["name"] = node->name;
        }
        content["attempts"] = retry->getAttempts();
        if (retry->hasChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(&(retry->getChild())));
            content["child"] = child_seq;
        }
        yaml_node["Retry"] = content;
    }
    else if (auto repeat = dynamic_cast<Repeat const*>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "repeat")
        {
            content["name"] = node->name;
        }
        content["times"] = repeat->getRepetitions();
        if (repeat->hasChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(&(repeat->getChild())));
            content["child"] = child_seq;
        }
        yaml_node["Repeat"] = content;
    }
    else if (auto until_success = dynamic_cast<UntilSuccess const*>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "repeat_until_success")
        {
            content["name"] = node->name;
        }
        if (until_success->hasChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(&(until_success->getChild())));
            content["child"] = child_seq;
        }
        yaml_node["RepeatUntilSuccess"] = content;
    }
    else if (auto until_failure = dynamic_cast<UntilFailure const*>(node))
    {
        YAML::Node content;
        if (!node->name.empty() && node->name != "repeat_until_failure")
        {
            content["name"] = node->name;
        }
        if (until_failure->hasChild())
        {
            YAML::Node child_seq;
            child_seq.push_back(generateYAMLNode(&(until_failure->getChild())));
            content["child"] = child_seq;
        }
        yaml_node["RepeatUntilFailure"] = content;
    }
    else if (auto condition = dynamic_cast<Condition const*>(node))
    {
        YAML::Node content;
        content["name"] = node->name;
        yaml_node["Condition"] = content;
    }
    else if (auto action = dynamic_cast<Action const*>(node))
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

// ----------------------------------------------------------------------------
void Exporter::generateBTCppXML(Node const* node,
                                std::stringstream& xml,
                                int indent)
{
    std::string spaces(static_cast<size_t>(indent), ' ');

    if (auto sequence = dynamic_cast<Sequence const*>(node))
    {
        xml << spaces << "<Sequence>" << std::endl;
        for (auto const& child : sequence->getChildren())
        {
            generateBTCppXML(child.get(), xml, indent + 2);
        }
        xml << spaces << "</Sequence>" << std::endl;
    }
    else if (auto selector = dynamic_cast<Selector const*>(node))
    {
        xml << spaces << "<Fallback>"
            << std::endl; // BT.CPP uses Fallback instead of Selector
        for (auto const& child : selector->getChildren())
        {
            generateBTCppXML(child.get(), xml, indent + 2);
        }
        xml << spaces << "</Fallback>" << std::endl;
    }
    else if (auto parallel = dynamic_cast<Parallel const*>(node))
    {
        xml << spaces << "<Parallel success_threshold=\""
            << parallel->getMinSuccess() << "\" failure_threshold=\""
            << parallel->getMinFail() << "\">" << std::endl;
        for (auto const& child : parallel->getChildren())
        {
            generateBTCppXML(child.get(), xml, indent + 2);
        }
        xml << spaces << "</Parallel>" << std::endl;
    }
    else if (auto parallel_all = dynamic_cast<ParallelAll const*>(node))
    {
        xml << spaces << "<Parallel success_threshold=\""
            << (parallel_all->getSuccessOnAll()
                    ? parallel_all->getChildren().size()
                    : 1)
            << "\" failure_threshold=\""
            << (parallel_all->getFailOnAll()
                    ? parallel_all->getChildren().size()
                    : 1)
            << "\">" << std::endl;
        for (auto const& child : parallel_all->getChildren())
        {
            generateBTCppXML(child.get(), xml, indent + 2);
        }
        xml << spaces << "</Parallel>" << std::endl;
    }
    else if (auto inverter = dynamic_cast<Inverter const*>(node))
    {
        xml << spaces << "<Inverter>" << std::endl;
        if (inverter->hasChild())
        {
            generateBTCppXML(&(inverter->getChild()), xml, indent + 2);
        }
        xml << spaces << "</Inverter>" << std::endl;
    }
    else if (auto repeat = dynamic_cast<Repeat const*>(node))
    {
        xml << spaces << "<RetryUntilSuccessful num_attempts=\""
            << repeat->getRepetitions() << "\">" << std::endl;
        if (repeat->hasChild())
        {
            generateBTCppXML(&(repeat->getChild()), xml, indent + 2);
        }
        xml << spaces << "</RetryUntilSuccessful>" << std::endl;
    }
    else if (auto action = dynamic_cast<Action const*>(node))
    {
        xml << spaces << "<Action ID=\"" << node->name << "\"/>" << std::endl;
    }
}

// ----------------------------------------------------------------------------
std::string Exporter::toMermaid(Tree const& tree)
{
    std::string result = "graph TD\n";

    // Define the styles for each type of node with classDef
    result += "    classDef sequence "
              "fill:#b3e0ff,stroke:#0066cc,stroke-width:2px,color:#000000,font-"
              "weight:bold\n";
    result += "    classDef selector "
              "fill:#ffcccc,stroke:#cc0000,stroke-width:2px,color:#000000,font-"
              "weight:bold\n";
    result += "    classDef parallel "
              "fill:#d9b3ff,stroke:#6600cc,stroke-width:2px,color:#000000,font-"
              "weight:bold\n";
    result += "    classDef decorator "
              "fill:#ffffb3,stroke:#cccc00,stroke-width:2px,color:#000000,font-"
              "weight:bold\n";
    result += "    classDef condition "
              "fill:#b3ffb3,stroke:#00cc00,stroke-width:2px,color:#000000,font-"
              "weight:bold\n";
    result += "    classDef action "
              "fill:#ffb3d9,stroke:#cc0066,stroke-width:2px,color:#000000,font-"
              "weight:bold\n";

    if (tree.hasRoot())
    {
        size_t counter = 0;
        generateMermaidNode(&(tree.getRoot()), 0, counter, result);
    }

    return result;
}

// ----------------------------------------------------------------------------
void Exporter::generateMermaidNode(Node const* node,
                                   size_t parent_id,
                                   size_t& counter,
                                   std::string& result)
{
    if (node == nullptr)
        return;

    size_t current_id = ++counter;
    std::string node_name = node->name;
    std::string node_class = "";

    if (dynamic_cast<Sequence const*>(node))
    {
        node_class = "sequence";
    }
    else if (dynamic_cast<Selector const*>(node))
    {
        node_class = "selector";
    }
    else if (dynamic_cast<Parallel const*>(node) ||
             dynamic_cast<ParallelAll const*>(node))
    {
        node_class = "parallel";
    }
    else if (dynamic_cast<Inverter const*>(node) ||
             dynamic_cast<Retry const*>(node) ||
             dynamic_cast<Repeat const*>(node) ||
             dynamic_cast<UntilSuccess const*>(node) ||
             dynamic_cast<UntilFailure const*>(node))
    {
        node_class = "decorator";
    }
    else if (dynamic_cast<Condition const*>(node))
    {
        node_class = "condition";
    }
    else if (dynamic_cast<Action const*>(node))
    {
        node_class = "action";
    }

    // Add the node definition
    result +=
        "    node" + std::to_string(current_id) + "[\"" + node_name + "\"]\n";

    // Apply the class to the node
    if (!node_class.empty())
    {
        result += "    class node" + std::to_string(current_id) + " " +
                  node_class + "\n";
    }

    // Add the connection to the parent (if it's not the root)
    if (parent_id > 0)
    {
        result += "    node" + std::to_string(parent_id) + " --> node" +
                  std::to_string(current_id) + "\n";
    }

    // Process the children for composite nodes
    if (auto composite = dynamic_cast<Composite const*>(node))
    {
        for (auto const& child : composite->getChildren())
        {
            generateMermaidNode(child.get(), current_id, counter, result);
        }
    }
    // Process the child for decorator nodes
    else if (auto decorator = dynamic_cast<Decorator const*>(node))
    {
        if (decorator->hasChild())
        {
            generateMermaidNode(
                &decorator->getChild(), current_id, counter, result);
        }
    }
}

} // namespace bt