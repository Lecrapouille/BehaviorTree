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
#include <algorithm>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace bt {

// ****************************************************************************
/// \brief Helper functions for node builders
// ****************************************************************************
namespace {

/// \brief Get the name of a node from YAML content
/// \param[in] content The YAML content containing the name
/// \return The name of the node or "unnamed" if not specified
std::string getNodeName(YAML::Node const& content)
{
    return content["name"] ? content["name"].as<std::string>()
                           : content.begin()->first.as<std::string>();
}

/// \brief Parse children nodes from YAML content
/// \param[in] factory The factory to create custom nodes
/// \param[in] content The YAML content containing the children
/// \param[in] field_name The name of the field containing children
/// \return The list of created child nodes
std::vector<Node::Ptr> parseChildren(NodeFactory const& factory,
                                     YAML::Node const& content,
                                     std::string const& field_name)
{
    if (!content[field_name])
    {
        throw std::runtime_error("Node '" + getNodeName(content) +
                                 "' missing '" + field_name + "' field");
    }

    if (!content[field_name].IsSequence())
    {
        throw std::runtime_error("Node '" + getNodeName(content) + "': '" +
                                 field_name + "' field must be a sequence");
    }

    if (content[field_name].size() == 0)
    {
        throw std::runtime_error("Node '" + getNodeName(content) +
                                 "' must have at least one child");
    }

    std::vector<Node::Ptr> children;
    for (auto const& child : content[field_name])
    {
        children.push_back(Builder::parseYAMLNode(factory, child));
    }
    return children;
}

/// \brief Factory for creating composite nodes (Sequence, Selector)
template <typename T>
class CompositeNodeCreator
{
public:

    static_assert(std::is_base_of_v<Composite, T>,
                  "T must inherit from bt::Composite");

    static Node::Ptr create(NodeFactory const& factory,
                            YAML::Node const& content)
    {
        auto node = Node::create<T>();
        node->name = getNodeName(content);

        auto children = parseChildren(factory, content, "children");
        for (auto& child : children)
        {
            node->addChild(std::move(child));
        }

        return node;
    }
};

/// \brief Factory for creating Parallel nodes
class ParallelNodeCreator
{
public:

    static Node::Ptr create(NodeFactory const& factory,
                            YAML::Node const& content)
    {
        bool has_policies = content["success_on_all"] || content["fail_on_all"];
        bool has_thresholds =
            content["success_threshold"] || content["failure_threshold"];

        if (has_policies && has_thresholds)
        {
            throw std::runtime_error(
                "Cannot specify both policies and thresholds");
        }
        else if (!has_policies && !has_thresholds)
        {
            throw std::runtime_error("Missing policies or thresholds");
        }
        else if (has_policies)
        {
            bool success_on_all = content["success_on_all"]
                                      ? content["success_on_all"].as<bool>()
                                      : true;
            bool fail_on_all = content["fail_on_all"]
                                   ? content["fail_on_all"].as<bool>()
                                   : true;
            auto par = Node::create<ParallelAll>(success_on_all, fail_on_all);
            par->name = getNodeName(content);
            auto children = parseChildren(factory, content, "children");
            for (auto& child : children)
            {
                par->addChild(std::move(child));
            }

            return par;
        }
        else
        {
            size_t success_threshold =
                content["success_threshold"]
                    ? content["success_threshold"].as<size_t>()
                    : 1;
            size_t failure_threshold =
                content["failure_threshold"]
                    ? content["failure_threshold"].as<size_t>()
                    : 1;
            auto par =
                Node::create<Parallel>(success_threshold, failure_threshold);
            par->name = getNodeName(content);
            auto children = parseChildren(factory, content, "children");
            for (auto& child : children)
            {
                par->addChild(std::move(child));
            }

            return par;
        }
    }
};

/// \brief Factory for creating decorator nodes
template <typename T>
class DecoratorNodeCreator
{
public:

    static Node::Ptr create(NodeFactory const& factory,
                            YAML::Node const& content)
    {
        Node::Ptr dec;
        if constexpr (std::is_same_v<T, Retry>)
        {
            size_t attempts =
                content["attempts"] ? content["attempts"].as<size_t>() : 3;
            dec = Node::create<Retry>(attempts);
        }
        else
        {
            dec = Node::create<T>();
        }

        dec->name = getNodeName(content);

        auto children = parseChildren(factory, content, "child");
        if (children.size() != 1)
            throw std::runtime_error("Decorator must have exactly one child");

        reinterpret_cast<Decorator*>(dec.get())->setChild(
            std::move(children[0]));
        return dec;
    }
};

/// \brief Factory for creating leaf nodes (Action/Condition)
class LeafNodeCreator
{
public:

    static Node::Ptr create(NodeFactory const& factory,
                            YAML::Node const& content)
    {
        if (!content["name"])
            throw std::runtime_error(content.begin()->first.as<std::string>() +
                                     " node missing 'name' field");

        std::string name = content["name"].as<std::string>();
        auto node = factory.createNode(name);
        if (!node)
            throw std::runtime_error("Failed to create " +
                                     content.begin()->first.as<std::string>() +
                                     " node: " + name);

        node->name = name;
        // TODO: handle parameters via blackboard
        return node;
    }
};

/// \brief Factory for creating Success nodes
class SuccessNodeCreator
{
public:

    static Node::Ptr create(NodeFactory const& factory,
                            YAML::Node const& content)
    {
        auto node = Node::create<Success>();
        node->name = getNodeName(content);
        return node;
    }
};

/// \brief Factory for creating Failure nodes
class FailureNodeCreator
{
public:

    static Node::Ptr create(NodeFactory const& factory,
                            YAML::Node const& content)
    {
        auto node = Node::create<Failure>();
        node->name = getNodeName(content);
        return node;
    }
};

using NodeCreatorFn = Node::Ptr (*)(NodeFactory const&, YAML::Node const&);
using NodeCreatorMap = std::unordered_map<std::string, NodeCreatorFn>;

//-----------------------------------------------------------------------------
NodeCreatorMap& getNodeCreators()
{
    static NodeCreatorMap creators;
    if (creators.empty())
    {
        creators["Sequence"] = &CompositeNodeCreator<Sequence>::create;
        creators["Selector"] = &CompositeNodeCreator<Selector>::create;
        creators["Parallel"] = &ParallelNodeCreator::create;
        creators["Inverter"] = &DecoratorNodeCreator<Inverter>::create;
        creators["Retry"] = &DecoratorNodeCreator<Retry>::create;
        creators["Repeat"] = &DecoratorNodeCreator<Repeat>::create;
        creators["RepeatUntilSuccess"] =
            &DecoratorNodeCreator<UntilSuccess>::create;
        creators["RepeatUntilFailure"] =
            &DecoratorNodeCreator<UntilFailure>::create;
        creators["ForceSuccess"] = &DecoratorNodeCreator<ForceSuccess>::create;
        creators["ForceFailure"] = &DecoratorNodeCreator<ForceFailure>::create;
        creators["Action"] = &LeafNodeCreator::create;
        creators["Condition"] = &LeafNodeCreator::create;
        creators["Success"] = &SuccessNodeCreator::create;
        creators["Failure"] = &FailureNodeCreator::create;
    }
    return creators;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
std::unique_ptr<Tree> Builder::fromFile(NodeFactory const& factory,
                                        std::string const& file_path)
{
    try
    {
        YAML::Node root = YAML::LoadFile(file_path);
        if (!root["BehaviorTree"])
        {
            throw std::runtime_error("Missing 'BehaviorTree' node in YAML");
        }

        auto tree = std::make_unique<Tree>();
        tree->setRoot(parseYAMLNode(factory, root["BehaviorTree"]));
        return tree;
    }
    catch (const YAML::Exception& e)
    {
        throw std::runtime_error("YAML parsing error: " +
                                 std::string(e.what()));
    }
}

//-----------------------------------------------------------------------------
std::unique_ptr<Tree> Builder::fromText(NodeFactory const& factory,
                                        std::string const& yaml_text)
{
    try
    {
        YAML::Node root = YAML::Load(yaml_text);
        if (!root["BehaviorTree"])
        {
            throw std::runtime_error(
                "Missing 'BehaviorTree' node in YAML text");
        }

        auto tree = std::make_unique<Tree>();
        tree->setRoot(parseYAMLNode(factory, root["BehaviorTree"]));
        return tree;
    }
    catch (const YAML::Exception& e)
    {
        throw std::runtime_error("YAML parsing error: " +
                                 std::string(e.what()));
    }
}

//-----------------------------------------------------------------------------
Node::Ptr Builder::parseYAMLNode(NodeFactory const& factory,
                                 YAML::Node const& node)
{
    if (!node.IsMap())
    {
        throw std::runtime_error("Invalid node format: must be a map");
    }

    auto it = node.begin();
    if (it == node.end())
    {
        throw std::runtime_error(
            "Empty YAML node: a node must contain at least one key defining "
            "its type (e.g. Sequence, Selector, Action)");
    }

    std::string type = it->first.as<std::string>();
    auto const& creators = getNodeCreators();
    auto fn_it = creators.find(type);
    if (fn_it == creators.end())
    {
        throw std::runtime_error("Builder::parseYAMLNode: Unknown node type: " +
                                 type);
    }

    return fn_it->second(factory, it->second);
}

} // namespace bt