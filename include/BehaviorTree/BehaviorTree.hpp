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
//
// This project is based on this initial project:
//
//   BrainTree - A C++ behavior tree single header library.
//   Copyright 2015-2018 Par Arvidsson. All rights reserved.
//   Licensed under the MIT license
//   (https://github.com/arvidsson/BrainTree/blob/master/LICENSE).
//
//*****************************************************************************

#ifndef BEHAVIOR_TREE_HPP
#  define BEHAVIOR_TREE_HPP

#  include <memory>
#  include <vector>
#  include <string>
#  include <unordered_map>
#  include <cassert>

namespace bt {

// ****************************************************************************
//! \brief
// ****************************************************************************
enum class Status
{
    INVALID,
    SUCCESS,
    FAILURE,
    RUNNING,
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class Blackboard
{
public:

    template<class T> using Entry = std::map<std::string, T>;
    using Ptr = std::shared_ptr<Blackboard>;

    ~BlackBoard() { erase(); }

    template<class T> void set(std::string const& key, T const& val)
    {
        entries<T>()[key] = val;
    }

    template<class T> T get(std::string const& key) const
    {
        return entries<T>().at(key);
    }

    template<class T>
    inline Entry<T>& entries() const
    {
        return m_maps<T>.at(this);
    }

private:

    template<class T>
    Entry<T>& entries()
    {
        auto it = m_maps<T>.find(this);
        if (it == std::end(m_maps<T>))
        {
            // Hold list of created heterogeneous maps for their destruction
            m_erase_functions.emplace_back([](BlackBoard& blackboard)
            {
                m_maps<T>.erase(&blackboard);
            });

            return m_maps<T>[this];
        }
        return it->second;
    }

    // Destroy all created heterogeneous stacks
    void erase()
    {
        for (auto&& erase_func : m_erase_functions)
        {
            erase_func(*this);
        }
    }

private:

    template<class T>
    static std::unordered_map<const BlackBoard*, Entry<T>> m_maps;

    std::vector<std::function<void(BlackBoard&)>> m_erase_functions;
};

template<class T>
std::unordered_map<const BlackBoard*, BlackBoard::Entry<T>> BlackBoard::m_maps;

// ****************************************************************************
//! \brief
// ****************************************************************************
class Node
{
public:

    using Ptr = std::shared_ptr<Node>;

    virtual ~Node() = default;

    inline void setBlackboard(Blackboard::Ptr blackboard)
    {
        m_blackboard = blackboard;
    }

    inline Blackboard::Ptr getBlackboard() const
    {
        return m_blackboard;
    }

    virtual Status update() = 0;
    virtual void initialize() {};
    virtual void terminate(Status /*status*/)
    {}

    Status tick()
    {
        if (m_status != Status::RUNNING)
        {
            initialize();
        }

        m_status = update();
        if (m_status != Status::RUNNING)
        {
            terminate(m_status);
        }

        return m_status;
    }

    inline bool isSuccess() const { return m_status == Status::SUCCESS; }
    inline bool isFailure() const { return m_status == Status::FAILURE; }
    inline bool isRunning() const { return m_status == Status::RUNNING; }
    inline bool isTerminated() const { return isSuccess() || isFailure(); }

    inline void reset()
    {
        m_status = Status::INVALID;
    }

protected:

    Status m_status = Status::INVALID;
    Blackboard::Ptr m_blackboard = nullptr;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class Composite : public Node
{
public:

    virtual ~Composite() = default;

    void addChild(Node::Ptr child)
    {
        m_children.push_back(child);
        m_iterator = m_children.begin();
    }

    inline bool hasChildren() const
    {
        return !m_children.empty();
    }

protected:

    std::vector<Node::Ptr> m_children;
    std::vector<Node::Ptr>::iterator m_iterator;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class Decorator : public Node
{
public:

    virtual ~Decorator() = default;

    inline void setChild(Node::Ptr node)
    {
        m_child = node;
    }

    inline bool hasChild() const
    {
        return m_child != nullptr;
    }

protected:

    Node::Ptr m_child = nullptr;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class Leaf : public Node
{
public:

    Leaf() = default;

    Leaf(Blackboard::Ptr blackboard)
        : m_blackboard(blackboard)
    {}

    virtual ~Leaf() = default;

    virtual Status update() = 0;

protected:

    Blackboard::Ptr m_blackboard;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class BehaviorTree : public Node
{
public:

    BehaviorTree()
    {
        m_blackboard = std::make_shared<Blackboard>();
    }

    BehaviorTree(const Node::Ptr& rootNode)
        : BehaviorTree()
    {
        m_root = rootNode;
    }

    virtual Status update() override
    {
        return m_root->tick();
    }

    inline void setRoot(const Node::Ptr& node)
    {
        m_root = node;
    }

private:

    Node::Ptr m_root = nullptr;
};

// ****************************************************************************
template <class Parent>
class DecoratorBuilder;

// ****************************************************************************
//! \brief
// ****************************************************************************
template <class Parent>
class CompositeBuilder
{
public:

    CompositeBuilder(Parent* parent, Composite* node)
        : m_parent(parent), m_node(node)
    {}

    template <class NodeType, typename... Args>
    CompositeBuilder<Parent> leaf(Args&&... args)
    {
        auto child = std::make_shared<NodeType>((args)...);
        child->setBlackboard(m_node->getBlackboard());
        m_node->addChild(child);
        return *this;
    }

    template <class CompositeType, typename... Args>
    CompositeBuilder<CompositeBuilder<Parent>> composite(Args&&... args)
    {
        auto child = std::make_shared<CompositeType>((args)...);
        child->setBlackboard(m_node->getBlackboard());
        m_node->addChild(child);
        return CompositeBuilder<CompositeBuilder<Parent>>(
            this, reinterpret_cast<CompositeType*>(child.get()));
    }

    template <class DecoratorType, typename... Args>
    DecoratorBuilder<CompositeBuilder<Parent>> decorator(Args&&... args)
    {
        auto child = std::make_shared<DecoratorType>((args)...);
        child->setBlackboard(m_node->getBlackboard());
        m_node->addChild(child);
        return DecoratorBuilder<CompositeBuilder<Parent>>(
            this, reinterpret_cast<DecoratorType*>(child.get()));
    }

    Parent& end()
    {
        return *m_parent;
    }

private:

    Parent* m_parent;
    Composite* m_node;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
template <class Parent>
class DecoratorBuilder
{
public:
    DecoratorBuilder(Parent* parent, Decorator* node)
        : m_parent(parent), m_node(node)
    {}

    template <class NodeType, typename... Args>
    DecoratorBuilder<Parent> leaf(Args&&... args)
    {
        auto child = std::make_shared<NodeType>((args)...);
        child->setBlackboard(m_node->getBlackboard());
        m_node->setChild(child);
        return *this;
    }

    template <class CompositeType, typename... Args>
    CompositeBuilder<DecoratorBuilder<Parent>> composite(Args&&... args)
    {
        auto child = std::make_shared<CompositeType>((args)...);
        child->setBlackboard(m_node->getBlackboard());
        m_node->setChild(child);
        return CompositeBuilder<DecoratorBuilder<Parent>>(
            this, reinterpret_cast<CompositeType*>(child.get()));
    }

    template <class DecoratorType, typename... Args>
    DecoratorBuilder<DecoratorBuilder<Parent>> decorator(Args&&... args)
    {
        auto child = std::make_shared<DecoratorType>((args)...);
        child->setBlackboard(m_node->getBlackboard());
        m_node->setChild(child);
        return DecoratorBuilder<DecoratorBuilder<Parent>>(
            this, reinterpret_cast<DecoratorType*>(child.get()));
    }

    Parent& end()
    {
        return *m_parent;
    }

private:

    Parent* m_parent;
    Decorator* m_node;
};

// ****************************************************************************
//! \brief The Selector composite ticks each child node in order. If a child
//! succeeds or runs, the selector returns the same status.  In the next tick,
//! it will try to run each child in order again.  If all children fails, only
//! then does the selector fail.
// ****************************************************************************
class Selector : public Composite
{
public:

    virtual void initialize() override
    {
        m_iterator = m_children.begin();
    }

    virtual Status update() override
    {
        assert(hasChildren() && "Composite has no children");

        while (m_iterator != m_children.end())
        {
            auto status = (*m_iterator)->tick();

            if (status != Status::FAILURE)
            {
                return status;
            }

            m_iterator++;
        }

        return Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief The Sequence composite ticks each child node in order. If a child
//! fails or runs, the sequence returns the same status.  In the next tick, it
//! will try to run each child in order again.  If all children succeeds, only
//! then does the sequence succeed.
// ****************************************************************************
class Sequence : public Composite
{
public:

    virtual void initialize() override
    {
        m_iterator = m_children.begin();
    }

    virtual Status update() override
    {
        assert(hasChildren() && "Composite has no children");

        while (m_iterator != m_children.end())
        {
            auto status = (*m_iterator)->tick();

            if (status != Status::SUCCESS)
            {
                return status;
            }

            m_iterator++;
        }

        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief The StatefulSelector composite ticks each child node in order, and
//! remembers what child it prevously tried to tick.  If a child succeeds or
//! runs, the stateful selector returns the same status.  In the next tick, it
//! will try to run the next child or start from the beginning again.  If all
//! children fails, only then does the stateful selector fail.
// ****************************************************************************
class StatefulSelector : public Composite
{
public:

    virtual Status update() override
    {
        assert(hasChildren() && "Composite has no children");

        while (m_iterator != m_children.end())
        {
            auto status = (*m_iterator)->tick();

            if (status != Status::FAILURE)
            {
                return status;
            }

            m_iterator++;
        }

        m_iterator = m_children.begin();
        return Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief The StatefulSequence composite ticks each child node in order, and
//! remembers what child it prevously tried to tick.  If a child fails or runs,
//! the stateful sequence returns the same status.  In the next tick, it will
//! try to run the next child or start from the beginning again.  If all
//! children succeeds, only then does the stateful sequence succeed.
// ****************************************************************************
class StatefulSequence : public Composite
{
public:

    virtual Status update() override
    {
        assert(hasChildren() && "Composite has no children");

        while (m_iterator != m_children.end())
        {
            auto status = (*m_iterator)->tick();

            if (status != Status::SUCCESS)
            {
                return status;
            }

            m_iterator++;
        }

        m_iterator = m_children.begin();
        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class ParallelSequence : public Composite
{
public:

    ParallelSequence(bool successOnAll = true, bool failOnAll = true)
        : m_useSuccessFailPolicy(true),
          m_successOnAll(successOnAll),
          m_failOnAll(failOnAll)
    {}

    ParallelSequence(int minSuccess, int minFail)
        : m_minSuccess(minSuccess),
          m_minFail(minFail)
    {}

    virtual Status update() override
    {
        assert(hasChildren() && "Composite has no children");

        int m_minimumSuccess = m_minSuccess;
        int m_minimumFail = m_minFail;

        if (m_useSuccessFailPolicy)
        {
            if (m_successOnAll)
            {
                m_minimumSuccess = m_children.size();
            }
            else
            {
                m_minimumSuccess = 1;
            }

            if (m_failOnAll)
            {
                m_minimumFail = m_children.size();
            }
            else
            {
                m_minimumFail = 1;
            }
        }

        int total_success = 0;
        int total_fail = 0;

        for (auto &child : m_children)
        {
            auto status = child->tick();
            if (status == Status::SUCCESS)
            {
                total_success++;
            }
            if (status == Status::FAILURE)
            {
                total_fail++;
            }
        }

        if (total_success >= m_minimumSuccess)
        {
            return Status::SUCCESS;
        }
        if (total_fail >= m_minimumFail)
        {
            return Status::FAILURE;
        }

        return Status::RUNNING;
    }

private:

    bool m_useSuccessFailPolicy = false;
    bool m_successOnAll = true;
    bool m_failOnAll = true;
    int m_minSuccess = 0;
    int m_minFail = 0;
};

// ****************************************************************************
//! \brief The Succeeder decorator returns success, regardless of what happens
//! to the child.
// ****************************************************************************
class Succeeder : public Decorator
{
public:

    virtual Status update() override
    {
        m_child->tick();
        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief The Failer decorator returns failure, regardless of what happens to
//! the child.
// ****************************************************************************
class Failer : public Decorator
{
public:

    virtual Status update() override
    {
        m_child->tick();
        return Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief The Inverter decorator inverts the child node's status, i.e. failure
//! becomes success and success becomes failure.  If the child runs, the
//! Inverter returns the status that it is running too.
// ****************************************************************************
class Inverter : public Decorator
{
public:

    virtual Status update() override
    {
        auto s = m_child->tick();
        if (s == Status::SUCCESS)
        {
            return Status::FAILURE;
        }
        else if (s == Status::FAILURE)
        {
            return Status::SUCCESS;
        }

        return s;
    }
};

// ****************************************************************************
//! \brief The Repeater decorator repeats infinitely or to a limit until the
//! child returns success.
// ****************************************************************************
class Repeater : public Decorator
{
public:

    Repeater(int limit = 0)
        : m_limit(limit)
    {}

    virtual void initialize() override
    {
        m_counter = 0;
    }

    virtual Status update() override
    {
        m_child->tick();

        if ((m_limit > 0) && (++m_counter == m_limit))
        {
            m_counter = m_limit;
            return Status::SUCCESS;
        }

        return Status::RUNNING;
    }

protected:

    int m_limit;
    int m_counter = 0;
};

// ****************************************************************************
//! \brief The UntilSuccess decorator repeats until the child returns success
//! and then returns success.
// ****************************************************************************
class UntilSuccess : public Decorator
{
public:

    virtual Status update() override
    {
        while (true)
        {
            auto status = m_child->tick();
            if (status == Status::SUCCESS)
            {
                return Status::SUCCESS;
            }
        }
    }
};

// ****************************************************************************
//! \brief The UntilFailure decorator repeats until the child returns fail and
//! then returns success.
// ****************************************************************************
class UntilFailure : public Decorator
{
public:

    virtual Status update() override
    {
        while (true)
        {
            auto status = m_child->tick();
            if (status == Status::FAILURE)
            {
                return Status::SUCCESS;
            }
        }
    }
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class Builder
{
public:

    Builder()
    {
        m_tree = std::make_shared<BehaviorTree>();
    }

    template <class NodeType, typename... Args>
    Builder leaf(Args&&... args)
    {
        m_root = std::make_shared<NodeType>((args)...);
        m_root->setBlackboard(m_tree->getBlackboard());
        return *this;
    }

    template <class CompositeType, typename... Args>
    CompositeBuilder<Builder> composite(Args&&... args)
    {
        m_root = std::make_shared<CompositeType>((args)...);
        m_root->setBlackboard(m_tree->getBlackboard());
        return CompositeBuilder<Builder>(
            this, reinterpret_cast<CompositeType*>(m_root.get()));
    }

    template <class DecoratorType, typename... Args>
    DecoratorBuilder<Builder> decorator(Args&&... args)
    {
        m_root = std::make_shared<DecoratorType>((args)...);
        m_root->setBlackboard(m_tree->getBlackboard());
        return DecoratorBuilder<Builder>(
            this, reinterpret_cast<DecoratorType*>(m_root.get()));
    }

    Node::Ptr build()
    {
        assert((m_root != nullptr) && "The Behavior Tree is empty!");
        m_tree->setRoot(m_root);
        return m_tree;
    }

private:

    Node::Ptr m_root;
    std::shared_ptr<BehaviorTree> m_tree;
};

} // namespace bt

#endif
