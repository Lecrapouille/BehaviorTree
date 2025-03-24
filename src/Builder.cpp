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

#include "BehaviorTree/Builder.hpp"

#include <yaml-cpp/yaml.h>

namespace bt {

// ----------------------------------------------------------------------------
std::unique_ptr<Tree> Builder::fromFile(NodeFactory const& p_factory,
                                        std::string const& p_file_path)
{
    try
    {
        YAML::Node root = YAML::LoadFile(p_file_path);
        if (!root["behavior_tree"])
        {
            throw std::runtime_error("Missing 'behavior_tree' node in YAML");
        }

        auto tree = std::make_unique<Tree>();
        tree->setRoot(parseYAMLNode(p_factory, root["behavior_tree"]));
        return tree;
    }
    catch (const YAML::Exception& e)
    {
        throw std::runtime_error("YAML parsing error: " +
                                 std::string(e.what()));
    }
}

// ----------------------------------------------------------------------------
std::unique_ptr<Tree> Builder::fromText(NodeFactory const& p_factory,
                                        std::string const& p_yaml_text)
{
    try
    {
        YAML::Node root = YAML::Load(p_yaml_text);
        if (!root["behavior_tree"])
        {
            throw std::runtime_error(
                "Missing 'behavior_tree' node in YAML text");
        }

        auto tree = std::make_unique<Tree>();
        tree->setRoot(parseYAMLNode(p_factory, root["behavior_tree"]));
        return tree;
    }
    catch (const YAML::Exception& e)
    {
        throw std::runtime_error("YAML parsing error: " +
                                 std::string(e.what()));
    }
}

// ----------------------------------------------------------------------------
Node::Ptr Builder::parseYAMLNode(NodeFactory const& p_factory,
                                 YAML::Node const& p_node)
{
    if (!p_node.IsMap())
    {
        throw std::runtime_error("Invalid node format: must be a map");
    }

    std::string type;
    YAML::Node node_content;

    // Find the type and content of the node
    for (const auto& it : p_node)
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
            throw std::runtime_error(
                "Sequence 'children' field must be a sequence");
        }
        if (node_content["children"].size() < 2)
        {
            throw std::runtime_error(
                "Sequence must have at least two children");
        }
        for (auto const& child : node_content["children"])
        {
            seq->addChild(parseYAMLNode(p_factory, child));
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
            throw std::runtime_error(
                "Selector 'children' field must be a sequence");
        }
        if (node_content["children"].size() < 2)
        {
            throw std::runtime_error(
                "Selector must have at least two children");
        }
        for (auto const& child : node_content["children"])
        {
            sel->addChild(parseYAMLNode(p_factory, child));
        }
        return sel;
    }
    else if (type == "parallel")
    {
        // Check if we want to use success/failure policies or thresholds
        bool has_policies =
            node_content["success_on_all"] || node_content["fail_on_all"];
        bool has_thresholds = node_content["success_threshold"] ||
                              node_content["failure_threshold"];

        Node::Ptr par;
        if (has_policies && has_thresholds)
        {
            throw std::runtime_error("Cannot specify both success/failure "
                                     "policies and thresholds policies");
        }
        else if (!has_policies && !has_thresholds)
        {
            throw std::runtime_error(
                "Missing success/failure policies or thresholds policies");
        }
        else if (has_policies)
        {
            bool success_on_all =
                node_content["success_on_all"]
                    ? node_content["success_on_all"].as<bool>()
                    : true;
            bool fail_on_all = node_content["fail_on_all"]
                                   ? node_content["fail_on_all"].as<bool>()
                                   : true;
            par = Node::create<ParallelAll>(success_on_all, fail_on_all);
        }
        else // has_thresholds
        {
            size_t success_threshold =
                node_content["success_threshold"]
                    ? node_content["success_threshold"].as<size_t>()
                    : size_t(1);
            size_t failure_threshold =
                node_content["failure_threshold"]
                    ? node_content["failure_threshold"].as<size_t>()
                    : size_t(1);
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
            throw std::runtime_error(
                "Parallel 'children' field must be a sequence");
        }
        if (node_content["children"].size() < 2)
        {
            throw std::runtime_error(
                "Parallel must have at least two children");
        }
        for (const auto& child : node_content["children"])
        {
            reinterpret_cast<Composite*>(par.get())->addChild(
                parseYAMLNode(p_factory, child));
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
            throw std::runtime_error(
                "Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        inv->setChild(parseYAMLNode(p_factory, node_content["child"][0]));
        return inv;
    }
    else if (type == "retry")
    {
        size_t attempts = node_content["attempts"]
                              ? node_content["attempts"].as<size_t>()
                              : size_t(0);
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
            throw std::runtime_error(
                "Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        retry->setChild(parseYAMLNode(p_factory, node_content["child"][0]));
        return retry;
    }
    else if (type == "repeat")
    {
        size_t times = node_content["times"]
                           ? node_content["times"].as<size_t>()
                           : size_t(0);
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
            throw std::runtime_error(
                "Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        repeat->setChild(parseYAMLNode(p_factory, node_content["child"][0]));
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
            throw std::runtime_error(
                "Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        rep->setChild(parseYAMLNode(p_factory, node_content["child"][0]));
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
            throw std::runtime_error(
                "Decorator 'child' field must be a sequence");
        }
        if (node_content["child"].size() != 1)
        {
            throw std::runtime_error("Decorator must have exactly one child");
        }
        rep->setChild(parseYAMLNode(p_factory, node_content["child"][0]));
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
        auto action = p_factory.createNode(action_name);
        if (!action)
        {
            throw std::runtime_error("Failed to create action node: " +
                                     action_name);
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
        auto condition = p_factory.createNode(condition_name);
        if (!condition)
        {
            throw std::runtime_error("Failed to create condition node: " +
                                     condition_name);
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

} // namespace bt