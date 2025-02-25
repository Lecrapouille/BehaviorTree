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

std::unique_ptr<Tree> TreeBuilder::fromText(std::string const& yaml_text)
{
    try
    {
        YAML::Node root = YAML::Load(yaml_text);
        if (!root["behavior_tree"])
        {
            throw std::runtime_error("Missing 'behavior_tree' node in YAML text");
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

    std::string type;
    YAML::Node node_content;

    // Find the type and content of the node
    for (const auto& it : node)
    {
        type = it.first.as<std::string>();
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        node_content = it.second;
        break;
    }

    if (type.empty())
    {
        throw std::runtime_error("Node missing type");
    }

    // Composite nodes
    if (type == "sequence")
    {
        auto seq = Node::create<Sequence>();
        if (node_content["name"])
        {
            seq->name = node_content["name"].as<std::string>();
        }
        else
        {
            seq->name = type;
        }
        if (!node_content["children"])
        {
            throw std::runtime_error("Sequence node missing 'children' field");
        }
        if (!node_content["children"].IsSequence())
        {
            throw std::runtime_error("Sequence 'children' field must be a sequence");
        }
        if (node_content["children"].size() < 2)
        {
            throw std::runtime_error("Sequence must have at least two children");
        }
        for (auto const& child : node_content["children"])
        {
            seq->addChild(parseYAMLNode(child));
        }
        return seq;
    }
    else if (type == "selector")
    {
        auto sel = Node::create<Selector>();
        if (node_content["name"])
        {
            sel->name = node_content["name"].as<std::string>();
        }
        else
        {
            sel->name = type;
        }
        if (!node_content["children"])
        {
            throw std::runtime_error("Selector node missing 'children' field");
        }
        if (!node_content["children"].IsSequence())
        {
            throw std::runtime_error("Selector 'children' field must be a sequence");
        }
        if (node_content["children"].size() < 2)
        {
            throw std::runtime_error("Selector must have at least two children");
        }
        for (auto const& child : node_content["children"])
        {
            sel->addChild(parseYAMLNode(child));
        }
        return sel;
    }
    else if (type == "parallel")
    {
        // Check if we want to use success/failure policies or thresholds
        bool has_policies = node_content["success_on_all"] || node_content["fail_on_all"];
        bool has_thresholds = node_content["success_threshold"] || node_content["failure_threshold"];

        Node::Ptr par;
        if (has_policies && has_thresholds)
        {
            throw std::runtime_error("Cannot specify both success/failure policies and thresholds policies");
        }
        else if (!has_policies && !has_thresholds)
        {
            throw std::runtime_error("Missing success/failure policies or thresholds policies");
        }
        else if (has_policies)
        {
            bool success_on_all = node_content["success_on_all"] ?
                node_content["success_on_all"].as<bool>() : true;
            bool fail_on_all = node_content["fail_on_all"] ?
                node_content["fail_on_all"].as<bool>() : true;
            par = Node::create<ParallelAll>(success_on_all, fail_on_all);
        }
        else // has_thresholds
        {
            size_t success_threshold = node_content["success_threshold"] ?
                node_content["success_threshold"].as<size_t>() : size_t(1);
            size_t failure_threshold = node_content["failure_threshold"] ?
                node_content["failure_threshold"].as<size_t>() : size_t(1);
            par = Node::create<Parallel>(success_threshold, failure_threshold);
        }

        if (node_content["name"])
        {
            par->name = node_content["name"].as<std::string>();
        }
        else
        {
            par->name = type;
        }

        if (!node_content["children"])
        {
            throw std::runtime_error("Parallel node missing 'children' field");
        }
        if (!node_content["children"].IsSequence())
        {
            throw std::runtime_error("Parallel 'children' field must be a sequence");
        }
        if (node_content["children"].size() < 2)
        {
            throw std::runtime_error("Parallel must have at least two children");
        }
        for (const auto& child : node_content["children"])
        {
            reinterpret_cast<Composite*>(par.get())->addChild(parseYAMLNode(child));
        }
        return par;
    }
    // Decorator nodes
    else if (type == "inverter")
    {
        auto inv = Node::create<Inverter>();
        if (node_content["name"])
        {
            inv->name = node_content["name"].as<std::string>();
        }
        else
        {
            inv->name = type;
        }
        if (!node_content["child"])
        {
            throw std::runtime_error("Decorator node missing 'child' field");
        }
        if (!node_content["child"].IsSequence())
        {
            throw std::runtime_error("Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        inv->setChild(parseYAMLNode(node_content["child"][0]));
        return inv;
    }
    else if (type == "retry")
    {
        size_t attempts = node_content["attempts"] ? 
            node_content["attempts"].as<size_t>() : size_t(0);
        auto retry = Node::create<Retry>(attempts);
        if (node_content["name"])
        {
            retry->name = node_content["name"].as<std::string>();
        }
        else
        {
            retry->name = type;
        }
        if (!node_content["child"])
        {
            throw std::runtime_error("Decorator node missing 'child' field");
        }
        if (!node_content["child"].IsSequence())
        {
            throw std::runtime_error("Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        retry->setChild(parseYAMLNode(node_content["child"][0]));
        return retry;
    }
    else if (type == "repeat")
    {
        size_t times = node_content["times"] ? 
            node_content["times"].as<size_t>() : size_t(0);
        auto repeat = Node::create<Repeat>(times);
        if (node_content["name"])
        {
            repeat->name = node_content["name"].as<std::string>();
        }
        else
        {
            repeat->name = type;
        }
        if (!node_content["child"])
        {
            throw std::runtime_error("Decorator node missing 'child' field");
        }
        if (!node_content["child"].IsSequence())
        {
            throw std::runtime_error("Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        repeat->setChild(parseYAMLNode(node_content["child"][0]));
        return repeat;
    }
    else if (type == "repeat_until_success")
    {
        auto rep = Node::create<UntilSuccess>();
        if (node_content["name"])
        {
            rep->name = node_content["name"].as<std::string>();
        }
        else
        {
            rep->name = type;
        }
        if (!node_content["child"])
        {
            throw std::runtime_error("Decorator node missing 'child' field");
        }
        if (!node_content["child"].IsSequence())
        {
            throw std::runtime_error("Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        rep->setChild(parseYAMLNode(node_content["child"][0]));
        return rep;
    }
    else if (type == "repeat_until_failure")
    {
        auto rep = Node::create<UntilFailure>();
        if (node_content["name"])
        {
            rep->name = node_content["name"].as<std::string>();
        }
        else
        {
            rep->name = type;
        }
        if (!node_content["child"])
        {
            throw std::runtime_error("Decorator node missing 'child' field");
        }
        if (!node_content["child"].IsSequence())
        {
            throw std::runtime_error("Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        rep->setChild(parseYAMLNode(node_content["child"][0]));
        return rep;
    }

    // Leaf nodes: Action and Condition nodes
    else if (type == "action")
    {
        if (!node_content["name"])
        {
            throw std::runtime_error("Action node missing 'name' field");
        }
        std::string action_name = node_content["name"].as<std::string>();
        auto action = m_factory->createNode(action_name);
        if (!action)
        {
            throw std::runtime_error("Failed to create action node: " + action_name);
        }
        action->name = action_name;
        
        // Parameters handling
        if (node_content["parameters"])
        {
            // TODO: implement parameters handling via the blackboard
        }
        return action;
    }
    else if (type == "condition")
    {
        if (!node_content["name"])
        {
            throw std::runtime_error("Condition node missing 'name' field");
        }
        std::string condition_name = node_content["name"].as<std::string>();
        auto condition = m_factory->createNode(condition_name);
        if (!condition)
        {
            throw std::runtime_error("Failed to create condition node: " + condition_name);
        }
        condition->name = condition_name;
        
        // Parameters handling
        if (node_content["parameters"])
        {
            // TODO: implement parameters handling via the blackboard
        }
        return condition;
    }

    throw std::runtime_error("Unknown node type: " + type);
}

std::string TreeBuilder::toMermaid(Tree const& tree)
{
    std::string result = "graph TD\n";
    
    // Définir les styles pour chaque type de nœud avec classDef
    result += "    classDef sequence fill:#b3e0ff,stroke:#0066cc,stroke-width:2px,color:#000000,font-weight:bold\n";
    result += "    classDef selector fill:#ffcccc,stroke:#cc0000,stroke-width:2px,color:#000000,font-weight:bold\n";
    result += "    classDef parallel fill:#d9b3ff,stroke:#6600cc,stroke-width:2px,color:#000000,font-weight:bold\n";
    result += "    classDef decorator fill:#ffffb3,stroke:#cccc00,stroke-width:2px,color:#000000,font-weight:bold\n";
    result += "    classDef condition fill:#b3ffb3,stroke:#00cc00,stroke-width:2px,color:#000000,font-weight:bold\n";
    result += "    classDef action fill:#ffb3d9,stroke:#cc0066,stroke-width:2px,color:#000000,font-weight:bold\n";
    
    size_t counter = 0;
    
    if (tree.getRoot() != nullptr)
    {
        generateMermaidNode(tree.getRoot().get(), 0, counter, result);
    }
    
    return result;
}

void TreeBuilder::generateMermaidNode(Node const* node, size_t parent_id,
                                    size_t& counter, std::string& result)
{
    if (node == nullptr)
        return;

    size_t current_id = ++counter;
    std::string node_name = node->name;
    std::string node_class = "";
    
    // Déterminer le type de nœud et la classe correspondante
    if (dynamic_cast<Sequence const*>(node))
    {
        node_class = "sequence";
    }
    else if (dynamic_cast<Selector const*>(node))
    {
        node_class = "selector";
    }
    else if (dynamic_cast<Parallel const*>(node) || dynamic_cast<ParallelAll const*>(node))
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
    
    // Ajouter la définition du nœud
    result += "    node" + std::to_string(current_id) + "[\"" + node_name + "\"]\n";
    
    // Appliquer la classe au nœud
    if (!node_class.empty())
    {
        result += "    class node" + std::to_string(current_id) + " " + node_class + "\n";
    }
    
    // Ajouter la connexion au parent (si ce n'est pas la racine)
    if (parent_id > 0)
    {
        result += "    node" + std::to_string(parent_id) + " --> node" +
                 std::to_string(current_id) + "\n";
    }
    
    // Traiter les enfants pour les nœuds composites
    if (auto composite = dynamic_cast<Composite const*>(node))
    {
        for (auto const& child : composite->getChildren())
        {
            generateMermaidNode(child.get(), current_id, counter, result);
        }
    }
    // Traiter l'enfant pour les nœuds décorateurs
    else if (auto decorator = dynamic_cast<Decorator const*>(node))
    {
        if (auto child = decorator->getChild())
        {
            generateMermaidNode(child.get(), current_id, counter, result);
        }
    }
}

} // namespace bt