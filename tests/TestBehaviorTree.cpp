//*****************************************************************************
// Unit tests for BehaviorTree
//*****************************************************************************

#include "BehaviorTree/TreeBuilder.hpp"
#include "BehaviorTree/TreeExporter.hpp"

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace bt {

// ****************************************************************************
//! \brief Mock action node for testing.
// ****************************************************************************
class MockAction : public Leaf
{
public:
    // ------------------------------------------------------------------------
    //! \brief Constructor with the desired result status.
    //! \param[in] result Status to return on tick.
    // ------------------------------------------------------------------------
    explicit MockAction(Status result)
        : m_result(result)
    {}

    // ------------------------------------------------------------------------
    virtual Status onRunning() override
    {
        return m_result;
    }

private:
    Status m_result;
};

// ****************************************************************************
//! \brief Test fixture for BehaviorTree tests.
// ****************************************************************************
class BehaviorTreeTest : public ::testing::Test
{
protected:
    // ------------------------------------------------------------------------
    //! \brief Create a dummy behavior tree.
    // ------------------------------------------------------------------------
    void SetUp() override
    {
        m_tree = std::make_unique<Tree>();
        m_blackboard = std::make_shared<Blackboard>();
    }

    // ------------------------------------------------------------------------
    //! \brief Helper to create a temporary YAML file.
    //! \param[in] content YAML content.
    //! \return Path to created file.
    // ------------------------------------------------------------------------
    std::string createTempYAML(std::string const& content)
    {
        std::string filename = "/tmp/temp_test.yaml";
        std::ofstream file(filename);
        file << content;
        file.close();
        return filename;
    }

protected:
    std::unique_ptr<Tree> m_tree;
    Blackboard::Ptr m_blackboard;
};

// ****************************************************************************
//! \brief Test Sequence node with success.
//! \details Verifies that Sequence node returns SUCCESS when all children succeed.
// ****************************************************************************
TEST_F(BehaviorTreeTest, SequenceNodeSuccess)
{
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::SUCCESS);

    // First tick: all children succeed
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    
    // Second tick: should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test Sequence node with failure.
//! \details Verifies that Sequence node returns FAILURE as soon as a child fails.
// ****************************************************************************
TEST_F(BehaviorTreeTest, SequenceNodeFailure)
{
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::FAILURE);
    seq.addChild<MockAction>(Status::SUCCESS); // Shall never be executed

    // First tick: fails on the second child
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    
    // Second tick: should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
}

// ****************************************************************************
//! \brief Test Selector node with success.
//! \details Verifies that Selector node returns SUCCESS as soon as a child succeeds.
// ****************************************************************************
TEST_F(BehaviorTreeTest, SelectorNodeSuccess)
{
    auto& sel = m_tree->setRoot<Selector>();
    sel.addChild<MockAction>(Status::FAILURE);
    sel.addChild<MockAction>(Status::SUCCESS);
    sel.addChild<MockAction>(Status::SUCCESS); // Shall never be executed

    // First tick: stops at the first success
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    
    // Second tick: should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test ParallelSequence node
//! \details Verifies ParallelSequence node behavior with minimum success threshold
// ****************************************************************************
TEST_F(BehaviorTreeTest, ParallelSequenceTest)
{
    size_t minSuccess = 2;
    size_t minFail = 2;
    auto& parallel = m_tree->setRoot<ParallelSequence>(minSuccess, minFail);
    parallel.addChild<MockAction>(Status::SUCCESS);
    parallel.addChild<MockAction>(Status::SUCCESS);
    parallel.addChild<MockAction>(Status::RUNNING);

    // First tick: reached the success threshold
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    
    // Test with failure
    parallel.reset();
    parallel.clearChildren();
    parallel.addChild<MockAction>(Status::FAILURE);
    parallel.addChild<MockAction>(Status::FAILURE);
    parallel.addChild<MockAction>(Status::SUCCESS);
    
    // Should fail because failure threshold reached
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
}

// ****************************************************************************
//! \brief Test Inverter node.
//! \details Verifies that Inverter correctly inverts its child's status.
// ****************************************************************************
TEST_F(BehaviorTreeTest, InverterTest)
{
    auto& inv = m_tree->setRoot<Inverter>();
    inv.setChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
}

// ****************************************************************************
//! \brief Test Repeater node.
//! \details Verifies that Repeater repeats its child's action the specified number of times.
// ****************************************************************************
TEST_F(BehaviorTreeTest, RepeaterTest)
{
    size_t limit = 2;
    auto& rep = m_tree->setRoot<Repeater>(limit);
    rep.setChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test basic tree loading from YAML.
//! \details Verifies behavior tree creation from a valid YAML file.
// ****************************************************************************
TEST_F(BehaviorTreeTest, YAMLBasicTree)
{
    std::string yaml = R"(
behavior_tree:
  type: sequence
  children:
    - type: always_success
      name: task1
    - type: selector
      children:
        - type: always_failure
          name: task2
        - type: always_success
          name: task3
)";

    auto factory = std::make_shared<NodeFactory>();
    // Register base node types
    factory->registerNode<AlwaysSuccess>("always_success");
    factory->registerNode<AlwaysFailure>("always_failure");
    factory->registerNode<Sequence>("sequence");
    factory->registerNode<Selector>("selector");
    
    TreeBuilder builder(factory);

    std::string filename = createTempYAML(yaml);
    auto tree = builder.fromYAML(filename);
    ASSERT_NE(tree, nullptr);
    std::remove(filename.c_str());
}

// ****************************************************************************
//! \brief Test loading from invalid YAML file.
//! \details Verifies that appropriate exception is thrown when loading non-existent file.
// ****************************************************************************
TEST_F(BehaviorTreeTest, YAMLInvalidFile)
{
    auto factory = std::make_shared<NodeFactory>();
    TreeBuilder builder(factory);

    EXPECT_THROW(builder.fromYAML("nonexistent.yaml"), std::runtime_error);
}

// ****************************************************************************
//! \brief Test basic Blackboard operations.
//! \details Verifies read/write operations on different data types.
// ****************************************************************************
TEST_F(BehaviorTreeTest, BlackboardTest)
{
    // Test set/get with different types
    m_blackboard->set("int_val", 42);
    m_blackboard->set("string_val", std::string("test"));
    m_blackboard->set("double_val", 3.14);

    // Test get with success
    auto [int_val, int_found] = m_blackboard->get<int>("int_val");
    EXPECT_TRUE(int_found);
    EXPECT_EQ(int_val, 42);

    // Test get with default value
    int default_val = m_blackboard->getOr<int>("missing_key", 100);
    EXPECT_EQ(default_val, 100);

    // Test invalid type
    auto [wrong_type, wrong_found] = m_blackboard->get<double>("int_val");
    EXPECT_FALSE(wrong_found);

    // Test empty key
    EXPECT_THROW(m_blackboard->set("", 42), std::invalid_argument);
}

// ****************************************************************************
//! \brief Test UntilSuccess node.
//! \details Verifies that node continues until success is achieved.
// ****************************************************************************
TEST_F(BehaviorTreeTest, UntilSuccessTest)
{
    class CountingAction : public Action
    {
    public:
        CountingAction() : m_count(0) {}
        
        virtual void onStart() override { m_status = Status::RUNNING; }
        
        Status onRunning() override {
            m_count++;
            return (m_count >= 2) ? Status::SUCCESS : Status::FAILURE;
        }
        
    private:
        int m_count;
    };

    auto& until = m_tree->setRoot<UntilSuccess>();
    until.setChild<CountingAction>();

    // First tick: the action fails, UntilSuccess continues
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    
    // Second tick: the action succeeds, UntilSuccess terminates
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test UntilFailure node.
//! \details Verifies that node continues until failure is achieved.
// ****************************************************************************
TEST_F(BehaviorTreeTest, UntilFailureTest)
{
    class CountingAction : public Action
    {
    public:
        CountingAction() : m_count(0) {}
        
        virtual void onStart() override { m_status = Status::RUNNING; }
        
        Status onRunning() override {
            m_count++;
            return (m_count >= 2) ? Status::FAILURE : Status::SUCCESS;
        }
        
    private:
        int m_count;
    };

    auto& until = m_tree->setRoot<UntilFailure>();
    until.setChild<CountingAction>();

    // First tick: the action succeeds, UntilFailure continues
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    
    // Second tick: the action fails, UntilFailure terminates
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test StatefulSelector node.
//! \details Verifies stateful behavior of Selector node.
// ****************************************************************************
TEST_F(BehaviorTreeTest, StatefulSelectorTest)
{
    auto& selector = m_tree->setRoot<StatefulSelector>();
    selector.addChild<MockAction>(Status::FAILURE);
    selector.addChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    selector.reset();
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test StatefulSequence node.
//! \details Verifies stateful behavior of Sequence node.
// ****************************************************************************
TEST_F(BehaviorTreeTest, StatefulSequenceTest)
{
    auto& seq = m_tree->setRoot<StatefulSequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::FAILURE);

    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    seq.reset();
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
}

// ****************************************************************************
//! \brief Test tree export.
//! \details Verifies tree export in YAML and XML formats.
// ****************************************************************************
TEST_F(BehaviorTreeTest, TreeExport)
{
    auto& root = m_tree->setRoot<Sequence>();
    root.addChild<MockAction>(Status::SUCCESS);
    
    // Test export YAML
    std::string yaml;
    EXPECT_NO_THROW(yaml = TreeExporter::toYAML(*m_tree));
    EXPECT_FALSE(yaml.empty());
    
    // Test export to YAML file
    EXPECT_NO_THROW(TreeExporter::toYAMLFile(*m_tree, "/tmp/test.yaml"));
    EXPECT_TRUE(std::filesystem::exists("/tmp/test.yaml"));
    std::filesystem::remove("/tmp/test.yaml");
}

// ****************************************************************************
//! \brief Test Action with Blackboard.
//! \details Verifies that an action can use a blackboard.
// ****************************************************************************
TEST_F(BehaviorTreeTest, ActionWithBlackboard)
{
    class TestAction : public Action 
    {
    public:
        TestAction(Blackboard::Ptr board) : Action(board) {}
        
        Status onRunning() override {
            m_blackboard->set("test_key", 42);
            return Status::SUCCESS;
        }
    };

    auto& root = m_tree->setRoot<TestAction>(m_blackboard);
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    
    auto [val, found] = m_blackboard->get<int>("test_key");
    EXPECT_TRUE(found);
    EXPECT_EQ(val, 42);
}

// ****************************************************************************
//! \brief Test NodeFactory registration.
//! \details Verifies that NodeFactory can register and retrieve actions.
// ****************************************************************************
TEST_F(BehaviorTreeTest, NodeFactoryRegistration)
{
    auto factory = std::make_shared<NodeFactory>();
    
    // Register a simple action
    factory->registerAction("TestAction", []() { return Status::SUCCESS; });
    
    // Register an action with a blackboard
    Blackboard::Ptr board = std::make_shared<Blackboard>();
    factory->registerAction("TestActionWithBoard", 
        [board]() {
            board->set("test", true);
            return Status::SUCCESS;
        },
        board);

    EXPECT_TRUE(factory->hasNode("TestAction"));
    EXPECT_TRUE(factory->hasNode("TestActionWithBoard"));
}

// ****************************************************************************
//! \brief Test Running behavior.
//! \details Verifies that the tree returns RUNNING when a child is RUNNING.
// ****************************************************************************
TEST_F(BehaviorTreeTest, RunningBehavior)
{
    class RunningAction : public Action
    {
    public:
        RunningAction() : m_ticks(0) {}
        
        Status onRunning() override {
            m_ticks++;
            return (m_ticks < 3) ? Status::RUNNING : Status::SUCCESS;
        }
        
    private:
        int m_ticks;
    };

    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<RunningAction>();
    seq.addChild<MockAction>(Status::SUCCESS);

    // The first two ticks should return RUNNING
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    
    // The third tick should complete the sequence
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test Tree reset.
//! \details Verifies that the tree resets correctly.
// ****************************************************************************
TEST_F(BehaviorTreeTest, TreeReset)
{
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::RUNNING);

    // The first tick should return RUNNING on the second child
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    
    // Reset the tree
    m_tree->reset();
    
    // Should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
}

// ****************************************************************************
//! \brief Test Tree halt.
//! \details Verifies that the tree halts correctly.
// ****************************************************************************
TEST_F(BehaviorTreeTest, TreeHalt)
{
    class HaltableAction : public Action
    {
    public:
        HaltableAction() : m_wasHalted(false) {}
        
        void onHalted() override {
            m_wasHalted = true;
        }
        
        bool wasHalted() const { return m_wasHalted; }
        
        Status onRunning() override {
            return Status::RUNNING;
        }
        
    private:
        bool m_wasHalted;
    };

    auto action = std::make_shared<HaltableAction>();
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild(action);

    // Start the action
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    
    // Halt should call onHalted
    m_tree->halt();
    EXPECT_TRUE(action->wasHalted());
}

} // namespace bt