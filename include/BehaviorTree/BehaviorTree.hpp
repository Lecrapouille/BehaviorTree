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
#  include <functional>

// FIXME: stocker les noeuds dans un containeur et utiliser que des raw pointeurs
// FIXME: ajouter parent ?
// generer le XML
// loader le XML
// faire mon groot

namespace bt {

// ****************************************************************************
//! \brief
// ****************************************************************************
enum class Status
{
    RUNNING = 1,
    SUCCESS = 2,
    FAILURE = 3,
};

inline std::string const& to_string(Status status)
{
  static std::string const names[3] = {"RUNNING", "SUCCESS", "FAILURE"};
  return names[int(status) - 1];
}

// ****************************************************************************
//! \brief
// ****************************************************************************
class Blackboard
{
public:

    template<class T> using Entry = std::unordered_map<std::string, T>;
    using Ptr = std::shared_ptr<Blackboard>;

    ~Blackboard() { erase(); }

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
            m_erase_functions.emplace_back([](Blackboard& blackboard)
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
    static std::unordered_map<const Blackboard*, Entry<T>> m_maps;

    std::vector<std::function<void(Blackboard&)>> m_erase_functions;
};

template<class T>
std::unordered_map<const Blackboard*, Blackboard::Entry<T>> Blackboard::m_maps;

// ****************************************************************************
//! \brief
// ****************************************************************************
class Node
{
public:

    using Ptr = std::shared_ptr<Node>;

    template<typename T, typename... Args>
    static std::unique_ptr<T> create(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    virtual ~Node() = default;

    inline void setBlackboard(Blackboard::Ptr blackboard)
    {
        m_blackboard = blackboard;
    }

    inline Blackboard::Ptr getBlackboard() const
    {
        return m_blackboard;
    }

    Status tick()
    {
        if (m_status != Status::RUNNING)
        {
            /*m_status =*/ onStart();
        }
        //else /*if (m_status == Status::RUNNING) ? */
        {
            m_status = onRunning();
            if (m_status != Status::RUNNING)
            {
                onHalted(m_status);
            }
        }
        return m_status;
    }

    inline bool isSuccess() const { return m_status == Status::SUCCESS; }
    inline bool isFailure() const { return m_status == Status::FAILURE; }
    inline bool isRunning() const { return m_status == Status::RUNNING; }
    inline bool isTerminated() const { return isSuccess() || isFailure(); }

    inline void reset()
    {
        m_status = Status(0);
    }

private:

    //! \brief Method called once, when transitioning from the state RUNNING.
    virtual /*Status*/ void onStart() { } //return Status::RUNNING; }

    //! \brief method invoked by the method tick() when the action is already in
    //! the RUNNING state.
    virtual Status onRunning() = 0;

    //! \brief when the method tick() is called and the action is no longer
    //! RUNNING, this method is invoked.  This is a convenient place for a
    //! cleanup, if needed.
    virtual void onHalted(Status /*status*/) {}

protected:

    Status m_status = Status(0);
    Blackboard::Ptr m_blackboard = nullptr;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class BehaviorTree : public Node
{
public:

    BehaviorTree()
    {
        m_blackboard = std::make_unique<Blackboard>();
    }

    template <class T, typename... Args>
    BehaviorTree(Args&&... args)
        : BehaviorTree()
    {
        m_root = Node::create<T>(std::forward<Args>(args)...);
    }

    Status tick()
    {
        m_status = m_root->tick();
        return m_status;
    }

    template <class T, typename... Args>
    inline T& setRoot(Args&&... args)
    {
        m_root = Node::create<T>(std::forward<Args>(args)...);
        return *reinterpret_cast<T*>(m_root.get());
    }

    inline Node& root()
    {
        return *m_root;
    }

private:

    virtual Status onRunning() override
    {
        m_status = m_root->tick();
        return m_status;
    }

private:

    Node::Ptr m_root = nullptr;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class Composite : public Node
{
public:

    virtual ~Composite() = default;

    template <class T, typename... Args>
    inline T& addChild(Args&&... args)
    {
        m_children.emplace_back(Node::create<T>(std::forward<Args>(args)...));
        m_iterator = m_children.begin();
        return *reinterpret_cast<T*>(m_children.back().get());
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

    template <class T, typename... Args>
    inline T& setChild(Args&&... args)
    {
        m_child = Node::create<T>(std::forward<Args>(args)...);
        return *reinterpret_cast<T*>(m_child.get());
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

protected:

    Blackboard::Ptr m_blackboard;
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

    virtual /*Status*/ void onStart() override
    {
        m_iterator = m_children.begin();
        //return Status::RUNNING;
    }

    virtual Status onRunning() override
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

    virtual /*Status*/ void onStart() override
    {
        m_iterator = m_children.begin();
        //return Status::RUNNING;
    }

    virtual Status onRunning() override
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

    virtual Status onRunning() override
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

    virtual Status onRunning() override
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

    virtual Status onRunning() override
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
//! \brief Simple leaf that always returns SUCCESS.
// ****************************************************************************
class AlwaysSuccess : public Leaf
{
public:

    virtual Status onRunning() override
    {
        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief Simple leaf that always returns FAILURE.
// ****************************************************************************
class AlwaysFailure : public Leaf
{
public:

    virtual Status onRunning() override
    {
        return Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief The Succeeder decorator returns success, regardless of what happens
//! to the child.
// ****************************************************************************
class Succeeder : public Decorator
{
public:

    virtual Status onRunning() override
    {
        m_status = m_child->tick();
        return isTerminated() ? Status::SUCCESS : m_status;
    }
};

// ****************************************************************************
//! \brief The Failer decorator returns failure, regardless of what happens to
//! the child.
// ****************************************************************************
class Failer : public Decorator
{
public:

    virtual Status onRunning() override
    {
        m_status = m_child->tick();
        return isTerminated() ? Status::FAILURE : m_status;
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

    virtual Status onRunning() override
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

    virtual /*Status*/ void onStart() override
    {
        m_counter = 0;
        //return Status::SUCCESS;
    }

    virtual Status onRunning() override
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

    virtual Status onRunning() override
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

    virtual Status onRunning() override
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

} // namespace bt

#endif
