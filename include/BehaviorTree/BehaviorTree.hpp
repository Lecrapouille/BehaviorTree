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
//
// This project is based on this initial project:
//
//   BrainTree - A C++ behavior tree single header library.
//   Copyright 2015-2018 Par Arvidsson. All rights reserved.
//   Licensed under the MIT license
//   (https://github.com/arvidsson/BrainTree/blob/master/LICENSE).
//
//*****************************************************************************

#pragma once

#include <algorithm>
#include <any>
#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace bt {

// ****************************************************************************
//! \brief Enum representing the status of a node in the behavior tree.
// ****************************************************************************
enum class Status
{
    RUNNING = 1,
    SUCCESS = 2,
    FAILURE = 3,
};

constexpr Status INVALID_STATUS = Status(0);

// ****************************************************************************
//! \brief Convert a node status to a string.
//! \param[in] status The status to convert.
//! \return The string representation of the status.
// ****************************************************************************
inline std::string const& to_string(Status status)
{
    static std::string const names[5] = {
        "INVALID", "RUNNING", "SUCCESS", "FAILURE", "???"};
    return names[int(status) < 4 ? int(status) : 4];
}

// ****************************************************************************
//! \brief Blackboard class allowing to store and share data between nodes of
//! different types using key-value pairs.
// ****************************************************************************
class Blackboard
{
public:

    template <class T>
    using Entry = std::unordered_map<std::string, T>;
    using Ptr = std::shared_ptr<Blackboard>;

    ~Blackboard() = default;

    // ------------------------------------------------------------------------
    //! \brief Set a value in the blackboard for a specific type.
    //! \param[in] key The key to set the value for.
    //! \param[in] val The value to set.
    // ------------------------------------------------------------------------
    template <typename T>
    void set(std::string const& key, T const& val)
    {
        if (key.empty())
            throw std::invalid_argument("Blackboard key cannot be empty");
        storage[key] = val;
    }

    // ------------------------------------------------------------------------
    //! \brief Get a value from the blackboard for a specific type.
    //! \param[in] key The key to get the value for.
    //! \return A pair containing the value and a boolean indicating if the
    //! value was found.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] std::pair<T, bool> get(std::string const& key) const
    {
        if (auto it = storage.find(key); it != storage.end())
        {
            try
            {
                return {std::any_cast<T>(it->second), true};
            }
            catch (const std::bad_any_cast&)
            {
                return {T{}, false};
            }
        }
        return {T{}, false};
    }

    // ------------------------------------------------------------------------
    //! \brief Get a value from the blackboard for a specific type with a
    //! default value. \param[in] key The key to get the value for. \param[in]
    //! defaultValue The default value to return if the key is not found.
    //! \return The value from the blackboard or the default value if the key is
    //! not found.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] T getOr(std::string const& key, const T& defaultValue) const
    {
        auto [value, found] = get<T>(key);
        return found ? value : defaultValue;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if a key exists for a specific type
    //! \param[in] key The key to check
    //! \return true if the key exists, false otherwise
    // ------------------------------------------------------------------------
    template <class T>
    [[nodiscard]] bool has(std::string const& key) const
    {
        auto it = m_maps<T>.find(this);
        if (it == m_maps<T>.end())
            return false;
        return it->second.find(key) != it->second.end();
    }

    // ------------------------------------------------------------------------
    //! \brief Remove a key-value pair from the blackboard
    //! \param[in] key The key to remove
    //! \return true if the key was removed, false if it didn't exist
    // ------------------------------------------------------------------------
    template <class T>
    bool remove(std::string const& key)
    {
        auto it = m_maps<T>.find(this);
        if (it == m_maps<T>.end())
            return false;
        return it->second.erase(key) > 0;
    }

    // ------------------------------------------------------------------------
    //! \brief Get all keys for a specific type
    //! \return Vector of keys stored for type T
    // ------------------------------------------------------------------------
    template <class T>
    [[nodiscard]] std::vector<std::string> getKeys() const
    {
        std::vector<std::string> keys;
        if (auto it = m_maps<T>.find(this); it != m_maps<T>.end())
        {
            keys.reserve(it->second.size());
            for (const auto& pair : it->second)
            {
                keys.push_back(pair.first);
            }
        }
        return keys;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the entries for a specific type
    //! \return The entries for the type
    // ------------------------------------------------------------------------
    template <class T>
    [[nodiscard]] inline Entry<T>& entries() const
    {
        return m_maps<T>.at(this);
    }

private:

    // ------------------------------------------------------------------------
    //! \brief The map of heterogeneous stacks
    // ------------------------------------------------------------------------
    template <class T>
    static std::unordered_map<const Blackboard*, Entry<T>> m_maps;

    // ------------------------------------------------------------------------
    //! \brief The map of heterogeneous stacks
    // ------------------------------------------------------------------------
    std::unordered_map<std::string, std::any> storage;
};

// ****************************************************************************
//! \brief Base class for all nodes in the behavior tree.
// ****************************************************************************
class Node
{
public:

    using Ptr = std::unique_ptr<Node>;

    // ------------------------------------------------------------------------
    //! \brief Create a new node of type T.
    //! \param[in] args The arguments to pass to the constructor of T.
    //! \return A unique pointer to the new node.
    // ------------------------------------------------------------------------
    template <typename T, typename... Args>
    [[nodiscard]] static std::unique_ptr<T> create(Args&&... args)
    {
        static_assert(std::is_base_of_v<Node, T>, "T must inherit from Node");
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // ------------------------------------------------------------------------
    //! \brief Destructor needed because of virtual methods.
    // ------------------------------------------------------------------------
    virtual ~Node() = default;

    // ------------------------------------------------------------------------
    //! \brief Execute the curent node.
    //! Call the onSetUp() method if the node was not running on previous tick.
    //! Call the onRunning() method if onSetUp() did not return FAILURE.
    //! Call the onTearDown() method if onRunning() did not return RUNNING.
    //! \return The status of the node (SUCCESS, FAILURE, RUNNING).
    // ------------------------------------------------------------------------
    [[nodiscard]] Status tick()
    {
        if (m_status != Status::RUNNING)
        {
            m_status = onSetUp();
        }
        if (m_status != Status::FAILURE)
        {
            m_status = onRunning();
            if (m_status != Status::RUNNING)
            {
                onTearDown(m_status);
            }
        }
        return m_status;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the status of the node.
    //! \return The status of the node.
    // ------------------------------------------------------------------------
    [[nodiscard]] inline Status getStatus() const
    {
        return m_status;
    }

    // ------------------------------------------------------------------------
    //! \brief Reset the status of the node to INVALID_STATUS. This will force
    //! the node to be re-initialized through the onSetUp() method on the
    //! next tick().
    // ------------------------------------------------------------------------
    virtual void reset()
    {
        m_status = INVALID_STATUS;
    }

    // ------------------------------------------------------------------------
    //! \brief Method invoked by the method onSetUp() of the Tree class to be
    //! sure the whole tree is valid.
    //! \return True if the node is valid, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool isValid() const = 0;

private:

    // ------------------------------------------------------------------------
    //! \brief Method invoked by the method tick(), when transitioning from the
    //! state RUNNING. This is a convenient place to setup the node when needed.
    //! \details By default nothing is done, override to handle the node
    //! startup logic.
    //! \return FAILURE if the node could not be initialized, else return
    //! SUCCESS or RUNNING if the node could be initialized.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onSetUp()
    {
        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Method invoked by the method tick() when the action is already in
    //! the RUNNING state.
    //! \details This method shall be overridden to handle the node running
    //! logic.
    //! \return The status of the node (SUCCESS, FAILURE, RUNNING).
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() = 0;

    // ------------------------------------------------------------------------
    //! \brief Method invoked by the method tick() when the action is no longer
    //! RUNNING. This is a convenient place for a cleanup the node when needed.
    //! \details By default nothing is done, override to handle the node
    //! cleanup logic.
    //! \param[in] status The status of the node (SUCCESS or FAILURE).
    // ------------------------------------------------------------------------
    virtual void onTearDown(Status p_status)
    {
        (void)p_status;
    }

public:

    //! \brief The name of the node.
    std::string name;

protected:

    //! \brief The status of the node.
    Status m_status = INVALID_STATUS;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class Tree: public Node
{
public:

    // ------------------------------------------------------------------------
    //! \brief Default constructor. You shall set the root node later before
    //! calling tick().
    // ------------------------------------------------------------------------
    Tree() = default;

    // ------------------------------------------------------------------------
    //! \brief Check if the tree is valid before starting the tree.
    //! \details The tree is valid if it has a root node and all nodes in the
    //! tree are valid (i.e. if decorators and composites have children).
    //! \return True if the tree is valid, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool isValid() const override
    {
        return (m_root != nullptr) && (m_root->isValid());
    }

    // ------------------------------------------------------------------------
    //! \brief Reset the tree.
    // ------------------------------------------------------------------------
    virtual void reset() override
    {
        m_root = nullptr;
        m_blackboard = nullptr;
        m_status = INVALID_STATUS;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the tree has a root node.
    //! \return True if the tree has a root node, false otherwise.
    // ------------------------------------------------------------------------
    inline bool hasRoot() const
    {
        return m_root != nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief Create and set the root node of the behavior tree.
    //! \param[in] p_args The arguments to pass to the constructor of T.
    //! \return A pointer to the new root node.
    // ------------------------------------------------------------------------
    template <class T, typename... Args>
    [[nodiscard]] inline Node& createRoot(Args&&... p_args)
    {
        m_root = Node::create<T>(std::forward<Args>(p_args)...);
        return *reinterpret_cast<T*>(m_root.get());
    }

    // ------------------------------------------------------------------------
    //! \brief Set the root node of the behavior tree.
    //! \param[in] p_root Pointer to the root node.
    // ------------------------------------------------------------------------
    void setRoot(Node::Ptr p_root)
    {
        m_root = std::move(p_root);
    }

    // ------------------------------------------------------------------------
    //! \brief Get the root node of the behavior tree.
    //! \return Const reference to the root node pointer.
    // ------------------------------------------------------------------------
    [[nodiscard]] Node const& getRoot() const
    {
        return *m_root;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the path of the node from the root node.
    //! \return The path of the node.
    // ------------------------------------------------------------------------
    // [[nodiscard]] Node const& getPath(std::string const& p_path) const
    // {
    //     return {}; // TODO
    // }

    // ------------------------------------------------------------------------
    //! \brief Set the blackboard of the tree
    //! \param[in] p_blackboard The blackboard to set
    // ------------------------------------------------------------------------
    void setBlackboard(Blackboard::Ptr p_blackboard)
    {
        m_blackboard = p_blackboard;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the blackboard of the tree
    //! \return A pointer to the blackboard
    // ------------------------------------------------------------------------
    [[nodiscard]] inline Blackboard::Ptr blackboard() const
    {
        return m_blackboard;
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Check if the tree is valid before starting the tree.
    //! \return The status of the tree: SUCCESS if the tree is valid,
    //! FAILURE otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onSetUp() override
    {
        // We need a root node to be able to tick the tree
        if (m_root == nullptr)
        {
            return Status::FAILURE;
        }

        // Check if the root node is valid
        if (!m_root->isValid())
        {
            return Status::FAILURE;
        }

        return Status::SUCCESS;
    }

    // ------------------------------------------------------------------------
    //! \brief Tick the root node of the tree.
    //! \return The status of the root node.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        assert(m_root != nullptr && "Root node is not set");
        m_status = m_root->tick();
        return m_status;
    }

private:

    //! \brief The root node of the behavior tree.
    Node::Ptr m_root = nullptr;
    //! \brief The blackboard for the node.
    Blackboard::Ptr m_blackboard = nullptr;
};

// ****************************************************************************
//! \brief Base class for composite nodes that can have multiple children.
//! Composite nodes are used to control the flow of the behavior tree.
// ****************************************************************************
class Composite: public Node
{
public:

    virtual ~Composite() = default;

    // ------------------------------------------------------------------------
    //! \brief Add an existing node as a child.
    //! \param[in] child Pointer to the child node to add.
    // ------------------------------------------------------------------------
    void addChild(Node::Ptr child)
    {
        m_children.emplace_back(std::move(child));
        m_iterator = m_children.begin();
    }

    // ------------------------------------------------------------------------
    //! \brief Create and add a new child node of type T.
    //! \param[in] args The arguments to pass to the constructor of T.
    //! \return A reference to the new child node.
    // ------------------------------------------------------------------------
    template <class T, typename... Args>
    [[nodiscard]] inline T& addChild(Args&&... args)
    {
        m_children.emplace_back(Node::create<T>(std::forward<Args>(args)...));
        m_iterator = m_children.begin();
        return *reinterpret_cast<T*>(m_children.back().get());
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the composite node has children.
    //! \return True if the composite node has children, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] inline bool hasChildren() const
    {
        return !m_children.empty();
    }

    // ------------------------------------------------------------------------
    //! \brief Get the children nodes
    //! \return Const reference to the vector of child nodes
    // ------------------------------------------------------------------------
    [[nodiscard]] std::vector<Node::Ptr> const& getChildren() const
    {
        return m_children;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the composite node is valid.
    //! \return True if the composite node is valid, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool isValid() const override
    {
        if (m_children.empty())
        {
            return false;
        }

        return std::all_of(m_children.begin(),
                           m_children.end(),
                           [](const auto& child) { return child->isValid(); });
    }

protected:

    //! \brief The children nodes of the composite node.
    std::vector<Node::Ptr> m_children;
    //! \brief The iterator to the current child node.
    std::vector<Node::Ptr>::iterator m_iterator;
};

// ****************************************************************************
//! \brief The Sequence composite ticks each child node in order. If a child
//! fails or runs, the sequence returns the same status.  In the next tick, it
//! will try to run each child in order again.  If all children succeeds, only
//! then does the sequence succeed.
// ****************************************************************************
class Sequence: public Composite
{
public:

    // ------------------------------------------------------------------------
    //! \brief Set up the sequence.
    //! \return The status of the sequence.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onSetUp() override
    {
        m_iterator = m_children.begin();
        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Run the sequence.
    //! \return The status of the sequence.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        while (m_iterator != m_children.end())
        {
            if (auto status = (*m_iterator)->tick(); status != Status::SUCCESS)
            {
                return status;
            }

            m_iterator++;
        }

        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief The ReactiveSequence composite ticks each child node in order. If a
//! child fails or runs, the reactive sequence returns the same status.  In the
//! next tick, it will try to run each child in order again.  If all children
//! succeeds, only then does the reactive sequence succeed.
// ****************************************************************************
class ReactiveSequence: public Composite
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the reactive sequence.
    //! \return The status of the reactive sequence.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        m_iterator = m_children.begin();
        while (m_iterator != m_children.end())
        {
            if (auto status = (*m_iterator)->tick(); status != Status::SUCCESS)
            {
                return status;
            }

            m_iterator++;
        }

        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief The StatefulSequence composite ticks each child node in order, and
//! remembers what child it previously tried to tick.  If a child fails or runs,
//! the stateful sequence returns the same status.  In the next tick, it will
//! try to run the next child or start from the beginning again.  If all
//! children succeeds, only then does the stateful sequence succeed.
// ****************************************************************************
class StatefulSequence
    : public Composite // Sequence with Memory → ContinueInOrder
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the stateful sequence.
    //! \return The status of the stateful sequence.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        while (m_iterator != m_children.end())
        {
            if (auto status = (*m_iterator)->tick(); status != Status::SUCCESS)
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
//! \brief The Selector (aka Fallback) composite ticks each child node in order.
//! If a child succeeds or runs, the selector returns the same status.  In the
//! next tick, it will try to run each child in order again.  If all children
//! fails, only then does the selector fail.
// ****************************************************************************
class Selector: public Composite
{
public:

    // ------------------------------------------------------------------------
    //! \brief Set up the selector.
    //! \return The status of the selector.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onSetUp() override
    {
        m_iterator = m_children.begin();
        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Run the selector.
    //! \return The status of the selector.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        while (m_iterator != m_children.end())
        {
            if (Status status = (*m_iterator)->tick();
                status != Status::FAILURE)
            {
                return status;
            }

            m_iterator++;
        }

        return Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief The ReactiveSelector composite ticks each child node in order. If a
//! child succeeds or runs, the reactive selector returns the same status.  In
//! the next tick, it will try to run each child in order again.  If all
//! children fails, only then does the reactive selector fail.
// ****************************************************************************
class ReactiveSelector: public Composite
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the reactive selector.
    //! \return The status of the reactive selector.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        m_iterator = m_children.begin();
        while (m_iterator != m_children.end())
        {
            if (Status status = (*m_iterator)->tick();
                status != Status::FAILURE)
            {
                return status;
            }

            m_iterator++;
        }

        return Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief The StatefulSelector composite ticks each child node in order, and
//! remembers what child it previously tried to tick.  If a child succeeds or
//! runs, the stateful selector returns the same status.  In the next tick, it
//! will try to run the next child or start from the beginning again.  If all
//! children fails, only then does the stateful selector fail.
// ****************************************************************************
class StatefulSelector: public Composite
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the stateful selector.
    //! \return The status of the stateful selector.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        while (m_iterator != m_children.end())
        {
            if (Status status = (*m_iterator)->tick();
                status != Status::FAILURE)
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
//! \brief The Parallel composite runs all children simultaneously.
//! It requires a minimum number of successful or failed children to determine
//! its own status.
// ****************************************************************************
class Parallel: public Composite
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor taking minimum success and failure counts.
    //! \param[in] minSuccess minimum successful children needed
    //! \param[in] minFail minimum failed children needed
    // ------------------------------------------------------------------------
    Parallel(int minSuccess, int minFail)
        : m_minSuccess(minSuccess), m_minFail(minFail)
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Get the minimum number of successful children needed.
    //! \return The minimum number of successful children needed.
    // ------------------------------------------------------------------------
    int getMinSuccess() const
    {
        return m_minSuccess;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the minimum number of failed children needed.
    //! \return The minimum number of failed children needed.
    // ------------------------------------------------------------------------
    int getMinFail() const
    {
        return m_minFail;
    }

    // ------------------------------------------------------------------------
    //! \brief Run the parallel composite.
    //! \return The status of the parallel composite.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        assert(hasChildren() && "Composite has no children");

        int total_success = 0;
        int total_fail = 0;

        for (auto const& child : m_children)
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

        if (total_success >= m_minSuccess)
        {
            return Status::SUCCESS;
        }
        if (total_fail >= m_minFail)
        {
            return Status::FAILURE;
        }

        return Status::RUNNING;
    }

private:

    int m_minSuccess;
    int m_minFail;
};

// ****************************************************************************
//! \brief The ParallelAll composite runs all children simultaneously.
//! It uses success/failure policies to determine its own status.
// ****************************************************************************
class ParallelAll: public Composite
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor taking success and failure policies.
    //! \param[in] successOnAll if true requires all children to succeed
    //! \param[in] failOnAll if true requires all children to fail
    // ------------------------------------------------------------------------
    ParallelAll(bool successOnAll = true, bool failOnAll = true)
        : m_successOnAll(successOnAll), m_failOnAll(failOnAll)
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Run the parallel all composite.
    //! \return The status of the parallel all composite.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        assert(hasChildren() && "Composite has no children");

        const size_t minimumSuccess = m_successOnAll ? m_children.size() : 1;
        const size_t minimumFail = m_failOnAll ? m_children.size() : 1;

        size_t total_success = 0;
        size_t total_fail = 0;

        for (auto const& child : m_children)
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

        if (total_success >= minimumSuccess)
        {
            return Status::SUCCESS;
        }
        if (total_fail >= minimumFail)
        {
            return Status::FAILURE;
        }

        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the success on all flag.
    //! \return The success on all flag.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool getSuccessOnAll() const
    {
        return m_successOnAll;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the fail on all flag.
    //! \return The fail on all flag.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool getFailOnAll() const
    {
        return m_failOnAll;
    }

private:

    bool m_successOnAll;
    bool m_failOnAll;
};

// ****************************************************************************
//! \brief Base class for decorator nodes that can have only one child.
//! Decorator nodes are used to modify the behavior of their child node.
// ****************************************************************************
class Decorator: public Node
{
public:

    // ------------------------------------------------------------------------
    //! \brief Set the child node of the decorator.
    //! \param[in] child The child node to set.
    // ------------------------------------------------------------------------
    void setChild(Node::Ptr p_child)
    {
        m_child = std::move(p_child);
    }

    // ------------------------------------------------------------------------
    //! \brief Set the child node of the decorator.
    //! \param[in] args The arguments to pass to the constructor of T
    //! \return A reference to the new child node
    // ------------------------------------------------------------------------
    template <class T, typename... Args>
    [[nodiscard]] inline T& setChild(Args&&... p_args)
    {
        m_child = Node::create<T>(std::forward<Args>(p_args)...);
        return *reinterpret_cast<T*>(m_child.get());
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the decorator has a child node.
    //! \return True if the decorator has a child node, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] inline bool hasChild() const
    {
        return m_child != nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the child node of the decorator.
    //! \note The child node shall exist. Use hasChild() to check.
    //! \return The child node reference.
    // ------------------------------------------------------------------------
    [[nodiscard]] inline Node const& getChild() const
    {
        return *(m_child.get());
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the composite node is valid.
    //! \return True if the composite node is valid, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool isValid() const override
    {
        return (m_child != nullptr) && (m_child->isValid());
    }

protected:

    Node::Ptr m_child = nullptr;
};

// ****************************************************************************
//! \brief The ForceSuccess decorator returns RUNNING if the child is RUNNING,
//! else returns SUCCESS, regardless of what happens to the child.
// ****************************************************************************
class ForceSuccess: public Decorator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the succeeder.
    //! \return The status of the succeeder.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        Status status = m_child->tick();
        return (status == Status::RUNNING) ? Status::RUNNING : Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief The ForceFailure decorator returns RUNNING if the child is RUNNING,
//! else returns FAILURE, regardless of what happens to the child.
// ****************************************************************************
class ForceFailure: public Decorator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the failer.
    //! \return The status of the failer.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        Status status = m_child->tick();
        return (status == Status::RUNNING) ? Status::RUNNING : Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief The Inverter decorator returns RUNNING if the child is RUNNING,
//! else returns the opposite of the child's status, i.e. FAILURE becomes
//! SUCCESS and SUCCESS becomes FAILURE.
// ****************************************************************************
class Inverter: public Decorator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the inverter.
    //! \return The status of the inverter.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
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
//! \fixme the loop should be done entirely internally during the tick() method
//! ?
// ****************************************************************************
class Repeat: public Decorator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor taking a limit of repetitions.
    //! \param[in] p_repetitions The limit of repetitions.
    // ------------------------------------------------------------------------
    explicit Repeat(size_t p_repetitions = 0) : m_repetitions(p_repetitions) {}

    // ------------------------------------------------------------------------
    //! \brief Set up the repeater.
    //! \return The status of the repeater.
    // ------------------------------------------------------------------------
    virtual Status onSetUp() override
    {
        m_count = 0;
        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Run the repeater.
    //! \fixme Should be a loop inside the tick() method like done in libBT.cpp?
    //! \return The status of the repeater.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        if (Status status = m_child->tick(); status == Status::RUNNING)
        {
            return Status::RUNNING;
        }
        else if (status == Status::FAILURE)
        {
            return Status::FAILURE;
        }

        if (m_repetitions > 0)
        {
            ++m_count;
            if (m_count == m_repetitions)
            {
                m_count = m_repetitions;
                return Status::SUCCESS;
            }
        }

        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the current count of repetitions.
    // ------------------------------------------------------------------------
    [[nodiscard]] size_t getCount() const
    {
        return m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the limit number of repetitions.
    // ------------------------------------------------------------------------
    [[nodiscard]] size_t getRepetitions() const
    {
        return m_repetitions;
    }

protected:

    size_t m_count = 0;
    size_t m_repetitions;
};

// ****************************************************************************
//! \brief The Retry decorator retries its child a specified number of times
//! until it succeeds or the maximum number of attempts is reached.
// ****************************************************************************
class Retry: public Decorator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor taking a number of attempts.
    //! \param[in] p_attempts The maximum number of attempts.
    // ------------------------------------------------------------------------
    explicit Retry(size_t p_attempts) : m_attempts(p_attempts) {}

    // ------------------------------------------------------------------------
    //! \brief Set up the retry.
    //! \return The status of the retry.
    // ------------------------------------------------------------------------
    virtual Status onSetUp() override
    {
        m_count = 0;
        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Run the retry.
    //! \return The status of the retry.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        if (Status status = m_child->tick(); status == Status::SUCCESS)
        {
            return Status::SUCCESS;
        }
        else if (status == Status::RUNNING)
        {
            return Status::RUNNING;
        }

        if (m_attempts > 0)
        {
            ++m_count;
            if (m_count >= m_attempts)
            {
                return Status::FAILURE;
            }
        }

        return Status::RUNNING;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the current count of attempts.
    //! \return The current count of attempts.
    // ------------------------------------------------------------------------
    [[nodiscard]] size_t getCount() const
    {
        return m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the limit number of attempts.
    //! \return The limit number of attempts.
    // ------------------------------------------------------------------------
    [[nodiscard]] size_t getAttempts() const
    {
        return m_attempts;
    }

protected:

    size_t m_count = 0;
    size_t m_attempts;
};

// ****************************************************************************
//! \brief The UntilSuccess decorator repeats until the child returns success
//! and then returns success.
// ****************************************************************************
class UntilSuccess: public Decorator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the until success decorator.
    //! \return The status of the until success decorator.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
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
class UntilFailure: public Decorator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Execute the decorator.
    //! \return The status of the decorator.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
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
//! \brief Base class for leaf nodes that have no children.
//! Leaf nodes are the nodes that actually do the work.
// ****************************************************************************
class Leaf: public Node
{
public:

    // ------------------------------------------------------------------------
    //! \brief Default constructor without blackboard.
    // ------------------------------------------------------------------------
    Leaf() = default;

    // ------------------------------------------------------------------------
    //! \brief Constructor with blackboard.
    // ------------------------------------------------------------------------
    explicit Leaf(Blackboard::Ptr p_blackboard) : m_blackboard(p_blackboard) {}

    // ------------------------------------------------------------------------
    //! \brief Get the blackboard for the node
    //! \return The blackboard for the node
    // ------------------------------------------------------------------------
    [[nodiscard]] inline Blackboard::Ptr getBlackboard() const
    {
        return m_blackboard;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the leaf node is valid.
    //! \return True if the leaf node is valid, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool isValid() const override
    {
        return true;
    }

protected:

    //! \brief The blackboard for the node
    Blackboard::Ptr m_blackboard = nullptr;
};

// ****************************************************************************
//! \brief Simple leaf that always returns SUCCESS.
// ****************************************************************************
class Success: public Leaf
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the always success leaf.
    //! \return The status of the always success leaf.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief Simple leaf that always returns FAILURE.
// ****************************************************************************
class Failure: public Leaf
{
public:

    // ------------------------------------------------------------------------
    //! \brief Run the always failure leaf.
    //! \return The status of the always failure leaf.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        return Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief Action node that can be used to execute custom behavior. You shall
//! override the onRunning() method to implement your behavior.
// ****************************************************************************
using Action = Leaf;

// ****************************************************************************
//! \brief Action node that can be used to execute custom behavior. This class
//! should not be used directly: it is used internally to sugar the class Action
//! by hiding inheritance.
// ****************************************************************************
class SugarAction: public Leaf
{
public:

    // ------------------------------------------------------------------------
    //! \brief Type alias for the action function.
    // ------------------------------------------------------------------------
    using Function = std::function<Status()>;

    // ------------------------------------------------------------------------
    //! \brief Constructor taking a function to execute.
    //! \param[in] func The function to execute when the action runs.
    // ------------------------------------------------------------------------
    SugarAction(Function func) : m_func(std::move(func)) {}

    // ------------------------------------------------------------------------
    //! \brief Constructor taking a function and blackboard.
    //! \param[in] func The function to execute when the action runs.
    //! \param[in] blackboard The blackboard to use.
    // ------------------------------------------------------------------------
    SugarAction(Function func, Blackboard::Ptr blackboard)
        : Leaf(blackboard), m_func(std::move(func))
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Execute the action.
    //! \return The status of the action.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        return m_func();
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the leaf node is valid (not nullptr function).
    //! \return True if the leaf node is valid, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool isValid() const override
    {
        return m_func != nullptr;
    }

private:

    //! \brief The function to execute when the action runs.
    Function m_func = nullptr;
};

// ****************************************************************************
//! \brief Condition node that can be used to evaluate a condition. This class
//! should not be used directly: it is used internally to sugar the class Action
//! by hiding inheritance.
// ****************************************************************************
class Condition: public Leaf
{
public:

    // ------------------------------------------------------------------------
    //! \brief Type alias for the condition function.
    // ------------------------------------------------------------------------
    using Function = std::function<bool()>;

    // ------------------------------------------------------------------------
    //! \brief Constructor taking a function to evaluate.
    //! \param[in] func The function to evaluate when the condition runs.
    // ------------------------------------------------------------------------
    Condition(Function func) : m_func(std::move(func)) {}

    // ------------------------------------------------------------------------
    //! \brief Constructor taking a function and blackboard.
    //! \param[in] func The function to evaluate when the condition runs.
    //! \param[in] blackboard The blackboard to use.
    // ------------------------------------------------------------------------
    Condition(Function func, Blackboard::Ptr blackboard)
        : Leaf(blackboard), m_func(std::move(func))
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Execute the condition.
    //! \return SUCCESS if the condition is true, FAILURE otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual Status onRunning() override
    {
        return m_func() ? Status::SUCCESS : Status::FAILURE;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if the leaf node is valid (not nullptr function).
    //! \return True if the leaf node is valid, false otherwise.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool isValid() const override
    {
        return m_func != nullptr;
    }

private:

    //! \brief The function to evaluate when the condition runs.
    Function m_func = nullptr;
};

// ****************************************************************************
//! \brief Factory class for creating behavior tree nodes.
//! This class allows registering custom node types that can be created by name.
// ****************************************************************************
class NodeFactory
{
public:

    // ------------------------------------------------------------------------
    //! \brief Function type for creating nodes.
    // ------------------------------------------------------------------------
    using NodeCreator = std::function<std::unique_ptr<Node>()>;

    // ------------------------------------------------------------------------
    //! \brief Default destructor.
    // ------------------------------------------------------------------------
    virtual ~NodeFactory() = default;

    // ------------------------------------------------------------------------
    //! \brief Register a node type with a creation function.
    //! \param[in] p_name Name used to identify this node type.
    //! \param[in] p_creator Function that creates instances of this node type.
    // ------------------------------------------------------------------------
    void registerNode(const std::string& p_name, NodeCreator p_creator)
    {
        m_creators[p_name] = std::move(p_creator);
    }

    // ------------------------------------------------------------------------
    //! \brief Create a node instance by name.
    //! \param[in] p_name The registered name of the node type to create.
    //! \return Unique pointer to the new node instance, or nullptr if name not
    //! found.
    // ------------------------------------------------------------------------
    std::unique_ptr<Node> createNode(const std::string& p_name) const
    {
        if (auto it = m_creators.find(p_name); it != m_creators.end())
        {
            return it->second();
        }
        return nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if a node type is registered.
    //! \param[in] p_name The name to check.
    //! \return True if the node type is registered.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool hasNode(const std::string& p_name) const
    {
        return m_creators.find(p_name) != m_creators.end();
    }

    // ------------------------------------------------------------------------
    //! \brief Helper method to register an action with a lambda.
    //! \param[in] p_name Name used to identify this action.
    //! \param[in] p_func Lambda function implementing the action.
    // ------------------------------------------------------------------------
    void registerAction(const std::string& p_name,
                        SugarAction::Function&& p_func)
    {
        registerNode(p_name, [func = std::move(p_func)]() {
            return Node::create<SugarAction>(func);
        });
    }

    // ------------------------------------------------------------------------
    //! \brief Helper method to register an action with a lambda and blackboard.
    //! \param[in] name Name used to identify this action.
    //! \param[in] p_func Lambda function implementing the action.
    //! \param[in] p_blackboard The blackboard to use.
    // ------------------------------------------------------------------------
    void registerAction(const std::string& p_name,
                        SugarAction::Function&& p_func,
                        Blackboard::Ptr p_blackboard)
    {
        registerNode(p_name, [func = std::move(p_func), p_blackboard]() {
            return Node::create<SugarAction>(func, p_blackboard);
        });
    }

    // ------------------------------------------------------------------------
    //! \brief Helper template method to register a node with blackboard.
    //! \tparam T The node type to register.
    //! \param[in] p_name Name used to identify this node type.
    //! \param[in] p_blackboard The blackboard to use.
    // ------------------------------------------------------------------------
    template <typename T>
    void registerNode(const std::string& p_name, Blackboard::Ptr p_blackboard)
    {
        registerNode(
            p_name, [p_blackboard]() { return Node::create<T>(p_blackboard); });
    }

    // ------------------------------------------------------------------------
    //! \brief Helper template method to register a node without blackboard.
    //! \tparam T The node type to register.
    //! \param[in] p_name Name used to identify this node type.
    // ------------------------------------------------------------------------
    template <typename T>
    void registerNode(const std::string& p_name)
    {
        registerNode(p_name, []() { return Node::create<T>(); });
    }

    // ------------------------------------------------------------------------
    //! \brief Helper method to register a condition with a lambda.
    //! \param[in] p_name Name used to identify this condition.
    //! \param[in] p_func Lambda function implementing the condition.
    // ------------------------------------------------------------------------
    void registerCondition(const std::string& p_name,
                           Condition::Function&& p_func)
    {
        registerNode(p_name, [func = std::move(p_func)]() {
            return Node::create<Condition>(func);
        });
    }

    // ------------------------------------------------------------------------
    //! \brief Helper method to register a condition with a lambda and
    //! blackboard.
    //! \param[in] p_name Name used to identify this condition.
    //! \param[in] p_func Lambda function implementing the condition.
    //! \param[in] p_blackboard The blackboard to use.
    // ------------------------------------------------------------------------
    void registerCondition(const std::string& p_name,
                           Condition::Function&& p_func,
                           Blackboard::Ptr p_blackboard)
    {
        registerNode(p_name, [func = std::move(p_func), p_blackboard]() {
            return Node::create<Condition>(func, p_blackboard);
        });
    }

protected:

    //! \brief Map of node names to their creation functions
    std::unordered_map<std::string, NodeCreator> m_creators;
};

} // namespace bt