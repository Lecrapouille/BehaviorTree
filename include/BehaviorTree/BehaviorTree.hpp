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
#  include <stdexcept>
#  include <optional>
#  include <any>

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

// ****************************************************************************
//! \brief Convert a node status to a string.
//! \param[in] status The status to convert.
//! \return The string representation of the status.
// ****************************************************************************
inline std::string const& to_string(Status status)
{
  static std::string const names[3] = {"RUNNING", "SUCCESS", "FAILURE"};
  return names[int(status) - 1];
}

// ****************************************************************************
//! \brief Blackboard class allowing to store and share data between nodes of
//! different types using key-value pairs.
// ****************************************************************************
class Blackboard
{
public:

    template<class T> using Entry = std::unordered_map<std::string, T>;
    using Ptr = std::shared_ptr<Blackboard>;

    ~Blackboard() { erase(); }

    // ------------------------------------------------------------------------
    //! \brief Set a value in the blackboard
    //! \param[in] key The key to store the value under
    //! \param[in] val The value to store
    //! \throw std::invalid_argument if key is empty
    // ------------------------------------------------------------------------
    template<class T>
    void set(std::string const& key, T const& val)
    {
        if (key.empty())
            throw std::invalid_argument("Blackboard key cannot be empty");
        entries<T>()[key] = val;
    }

    // ------------------------------------------------------------------------
    //! \brief Get a value from the blackboard
    //! \param[in] key The key to look up
    //! \return The value if found, empty pair if not found
    // ------------------------------------------------------------------------
    template<typename T>
    std::pair<T, bool> get(std::string const& key) const {
        auto it = storage.find(key);
        if (it != storage.end()) {
            try {
                return {std::any_cast<T>(it->second), true};
            } catch (const std::bad_any_cast&) {
                return {T{}, false};
            }
        }
        return {T{}, false};
    }

    // ------------------------------------------------------------------------
    //! \brief Get a value from the blackboard with a default value
    //! \param[in] key The key to look up
    //! \param[in] defaultValue The value to return if key not found
    //! \return The value from the blackboard or the default value
    // ------------------------------------------------------------------------
    template<typename T>
    T getOr(std::string const& key, const T& defaultValue) const {
        auto [value, found] = get<T>(key);
        return found ? value : defaultValue;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if a key exists for a specific type
    //! \param[in] key The key to check
    //! \return true if the key exists, false otherwise
    // ------------------------------------------------------------------------
    template<class T>
    bool has(std::string const& key) const
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
    template<class T>
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
    template<class T>
    std::vector<std::string> getKeys() const
    {
        std::vector<std::string> keys;
        auto it = m_maps<T>.find(this);
        if (it != m_maps<T>.end()) {
            keys.reserve(it->second.size());
            for (const auto& pair : it->second) {
                keys.push_back(pair.first);
            }
        }
        return keys;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the entries for a specific type
    //! \return The entries for the type
    // ------------------------------------------------------------------------
    template<class T>
    inline Entry<T>& entries() const
    {
        return m_maps<T>.at(this);
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Get the entries for a specific type
    //! \return The entries for the type
    // ------------------------------------------------------------------------
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

    // ------------------------------------------------------------------------
    //! \brief Destroy all created heterogeneous stacks
    // ------------------------------------------------------------------------
    void erase()
    {
        for (auto&& erase_func : m_erase_functions)
        {
            erase_func(*this);
        }
    }

private:

    // ------------------------------------------------------------------------
    //! \brief The map of heterogeneous stacks
    // ------------------------------------------------------------------------
    template<class T>
    static std::unordered_map<const Blackboard*, Entry<T>> m_maps;

    // ------------------------------------------------------------------------
    //! \brief The vector of functions to erase the heterogeneous stacks
    // ------------------------------------------------------------------------
    std::vector<std::function<void(Blackboard&)>> m_erase_functions;

    // ------------------------------------------------------------------------
    //! \brief The map of heterogeneous stacks
    // ------------------------------------------------------------------------
    std::unordered_map<std::string, std::any> storage;
};

template<class T>
std::unordered_map<const Blackboard*, Blackboard::Entry<T>> Blackboard::m_maps;

// ****************************************************************************
//! \brief Base class for all nodes in the behavior tree.
// ****************************************************************************
class Node
{
public:

    using Ptr = std::shared_ptr<Node>;

    // ------------------------------------------------------------------------
    //! \brief Create a new node of type T
    //! \param[in] args The arguments to pass to the constructor of T
    //! \return A unique pointer to the new node
    // ------------------------------------------------------------------------
    template<typename T, typename... Args>
    static std::unique_ptr<T> create(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // ------------------------------------------------------------------------
    //! \brief Destructor
    // ------------------------------------------------------------------------
    virtual ~Node() = default;

    // ------------------------------------------------------------------------
    //! \brief Set the blackboard for the node
    // ------------------------------------------------------------------------
    inline void setBlackboard(Blackboard::Ptr blackboard)
    {
        m_blackboard = blackboard;
    }

    // ------------------------------------------------------------------------
    //! \brief Get the blackboard for the node
    //! \return The blackboard for the node
    // ------------------------------------------------------------------------
    inline Blackboard::Ptr getBlackboard() const
    {
        return m_blackboard;
    }

    // ------------------------------------------------------------------------
    //! \brief Tick the node
    //! \return The status of the node
    // ------------------------------------------------------------------------
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

    // ------------------------------------------------------------------------
    //! \brief Check if the node is successful
    //! \return True if the node is successful, false otherwise
    // ------------------------------------------------------------------------
    inline bool isSuccess() const { return m_status == Status::SUCCESS; }

    // ------------------------------------------------------------------------
    //! \brief Check if the node is failed
    //! \return True if the node is failed, false otherwise
    // ------------------------------------------------------------------------
    inline bool isFailure() const { return m_status == Status::FAILURE; }

    // ------------------------------------------------------------------------
    //! \brief Reset the node
    // ------------------------------------------------------------------------
    inline void reset()
    {
        m_status = Status(0);
    }

protected:

    //! \brief The status of the node
    Status m_status = Status(0);

    //! \brief The blackboard for the node
    Blackboard::Ptr m_blackboard = nullptr;

    // Add this helper method
    bool isTerminated() const
    {
        return (m_status == Status::SUCCESS) || (m_status == Status::FAILURE);
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Method called once, when transitioning from the state RUNNING.
    // ------------------------------------------------------------------------
    virtual /*Status*/ void onStart() { } //return Status::RUNNING; }

    // ------------------------------------------------------------------------
    //! \brief Method invoked by the method tick() when the action is already in
    //! the RUNNING state.
    // ------------------------------------------------------------------------
    virtual Status onRunning() = 0;

    // ------------------------------------------------------------------------
    //! \brief when the method tick() is called and the action is no longer
    //! RUNNING, this method is invoked.  This is a convenient place for a
    //! cleanup, if needed.
    // ------------------------------------------------------------------------
    virtual void onHalted(Status /*status*/) {}
};

// ****************************************************************************
//! \brief Factory class for creating behavior tree nodes.
//! This class allows registering custom node types that can be created by name.
// ****************************************************************************
class NodeFactory
{
public:
    //! \brief Function type for creating nodes
    using NodeCreator = std::function<std::unique_ptr<Node>()>;

    virtual ~NodeFactory() = default;

    //! \brief Register a node type with a creation function
    //! \param[in] name Name used to identify this node type
    //! \param[in] creator Function that creates instances of this node type
    void registerNode(const std::string& name, NodeCreator creator)
    {
        m_creators[name] = std::move(creator);
    }

    //! \brief Create a node instance by name
    //! \param[in] name The registered name of the node type to create
    //! \return Unique pointer to the new node instance, or nullptr if name not found
    std::unique_ptr<Node> createNode(const std::string& name) const
    {
        auto it = m_creators.find(name);
        if (it != m_creators.end()) {
            return it->second();
        }
        return nullptr;
    }

    //! \brief Check if a node type is registered
    //! \param[in] name The name to check
    //! \return True if the node type is registered
    bool hasNode(const std::string& name) const
    {
        return m_creators.find(name) != m_creators.end();
    }

protected:
    //! \brief Map of node names to their creation functions
    std::unordered_map<std::string, NodeCreator> m_creators;
};

// ****************************************************************************
//! \brief
// ****************************************************************************
class BehaviorTree : public Node
{
public:

    //! \brief Default constructor
    BehaviorTree() = default;

    //! \brief Set the root node of the behavior tree
    //! \param[in] root Pointer to the root node
    void setRoot(Node::Ptr root)
    {
        m_root = std::move(root);
    }

    //! \brief Get the root node of the behavior tree
    //! \return Const reference to the root node pointer
    Node::Ptr const& getRoot() const
    {
        return m_root;
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
//! \brief Base class for composite nodes that can have multiple children.
//! Composite nodes are used to control the flow of the behavior tree.
// ****************************************************************************
class Composite : public Node
{
public:

    virtual ~Composite() = default;

    //! \brief Add an existing node as a child
    //! \param[in] child Pointer to the child node to add
    void addChild(Node::Ptr child)
    {
        m_children.emplace_back(std::move(child));
        m_iterator = m_children.begin();
    }

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

    //! \brief Get the children nodes
    //! \return Const reference to the vector of child nodes
    std::vector<Node::Ptr> const& children() const
    {
        return m_children;
    }

protected:

    std::vector<Node::Ptr> m_children;
    std::vector<Node::Ptr>::iterator m_iterator;
};

// ****************************************************************************
//! \brief Base class for decorator nodes that can have only one child.
//! Decorator nodes are used to modify the behavior of their child node.
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

    /**
     * @brief Get the child node
     * @return The child node pointer
     */
    Node::Ptr getChild() const { return m_child; }

protected:

    Node::Ptr m_child = nullptr;
};

// ****************************************************************************
//! \brief Base class for leaf nodes that have no children.
//! Leaf nodes are the nodes that actually do the work.
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
//! \brief The ParallelSequence composite runs all children simultaneously.
//! It can be configured with either success/failure policies or minimum counts.
//! When using policies:
//! - successOnAll=true requires all children to succeed
//! - failOnAll=true requires all children to fail
//! When using counts:
//! - minSuccess defines minimum successful children needed
//! - minFail defines minimum failed children needed
// ****************************************************************************
class ParallelSequence : public Composite
{
public:

    //! \brief Constructor with success/fail policy
    //! \param[in] successOnAll If true, requires all children to succeed
    //! \param[in] failOnAll If true, requires all children to fail
    explicit ParallelSequence(bool successOnAll = true, bool failOnAll = true)
        : m_useSuccessFailPolicy(true),
          m_successOnAll(successOnAll),
          m_failOnAll(failOnAll)
    {}

    //! \brief Constructor with minimum success/fail counts
    //! \param[in] minSuccess Minimum number of successful children required
    //! \param[in] minFail Minimum number of failed children required
    explicit ParallelSequence(size_t minSuccess, size_t minFail)
        : m_useSuccessFailPolicy(false),
          m_minSuccess(minSuccess),
          m_minFail(minFail)
    {}

    virtual Status onRunning() override
    {
        assert(hasChildren() && "Composite has no children");

        size_t m_minimumSuccess = m_minSuccess;
        size_t m_minimumFail = m_minFail;

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

        size_t total_success = 0;
        size_t total_fail = 0;

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
    size_t m_minSuccess = 0;
    size_t m_minFail = 0;
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

    /**
     * @brief Get the repeat limit
     * @return The number of repetitions
     */
    int getLimit() const { return m_limit; }

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
