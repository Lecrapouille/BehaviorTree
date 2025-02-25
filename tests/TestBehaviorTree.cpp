//*****************************************************************************
// Unit tests comparing with BehaviorTree.CPP
//*****************************************************************************

#include "BehaviorTree/BehaviorTree.hpp"
#include <behaviortree_cpp/behavior_tree.h>
#include <behaviortree_cpp/bt_factory.h>
#include <gtest/gtest.h>

namespace bt {

// ****************************************************************************
//! \brief Test fixture for comparing with BehaviorTree.CPP.
// ****************************************************************************
class CompareBehaviorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_tree = std::make_unique<Tree>();
        m_blackboard = std::make_shared<Blackboard>();
    }

protected:
    std::unique_ptr<Tree> m_tree;
    Blackboard::Ptr m_blackboard;
};

// ****************************************************************************
//! \brief Test invalid tree with null root.
// ****************************************************************************
TEST_F(CompareBehaviorTest, InvalidTreeRootNull)
{
    auto tree = std::make_unique<Tree>();
    EXPECT_EQ(tree->isValid(), false);
    EXPECT_EQ(tree->getStatus(), INVALID_STATUS);
}

// ****************************************************************************
//! \brief Test invalid tree with missing child.
// ****************************************************************************
TEST_F(CompareBehaviorTest, InvalidTreeMissingChild)
{
    auto tree = std::make_unique<Tree>();
    tree->setRoot<Inverter>();
    EXPECT_EQ(tree->isValid(), false);
    EXPECT_EQ(tree->getStatus(), INVALID_STATUS);
}

// ****************************************************************************
//! \brief Test invalid tree with missing children.
// ****************************************************************************
TEST_F(CompareBehaviorTest, InvalidTreeMissingChildren)
{
    auto tree = std::make_unique<Tree>();
    tree->setRoot<Sequence>();
    EXPECT_EQ(tree->isValid(), false);
    EXPECT_EQ(tree->getStatus(), INVALID_STATUS);
}

// ****************************************************************************
//! \brief Test invalid tree with missing children.
// ****************************************************************************
TEST_F(CompareBehaviorTest, InvalidTreeMissingChildrenDepth2)
{
    auto tree = std::make_unique<Tree>();
    auto& seq = tree->setRoot<Sequence>();
    seq.addChild<Inverter>();
    EXPECT_EQ(tree->isValid(), false);
    EXPECT_EQ(tree->getStatus(), INVALID_STATUS);
}

// ****************************************************************************
//! \brief Test Sequence node with success.
//! \details Verifies that Sequence node returns SUCCESS when all children succeed.
//! \note Compare Sequence node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, SequenceNodeSuccess)
{
    // Our implementation
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<AlwaysSuccess>();
    seq.addChild<AlwaysSuccess>();
    EXPECT_EQ(m_tree->isValid(), true);
    EXPECT_EQ(m_tree->getStatus(), INVALID_STATUS);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <root BTCPP_format="4">
            <BehaviorTree ID="Untitled">
                <Sequence>
                    <AlwaysSuccess/>
                    <AlwaysSuccess/>
                </Sequence>
            </BehaviorTree>
        </root>)";

    auto btcpp_tree = factory.createTreeFromText(xml_text);
   // EXPECT_EQ(btcpp_tree.status(), BT::NodeStatus::IDLE);

    // First tick: all children succeed
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(m_tree->getStatus(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);
    //EXPECT_EQ(btcpp_tree.status(), BT::NodeStatus::SUCCESS);

    // Second tick: should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(m_tree->getStatus(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);
    //EXPECT_EQ(btcpp_tree.status(), BT::NodeStatus::SUCCESS);
}

// ****************************************************************************
//! \brief Test Sequence node with failure.
//! \details Verifies that Sequence node returns FAILURE as soon as a child fails.
//! \note Compare Sequence node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, SequenceNodeFailure)
{
    // Our implementation
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<AlwaysSuccess>();
    seq.addChild<AlwaysFailure>();
    seq.addChild<AlwaysSuccess>(); // Shall never be executed
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <root BTCPP_format="4">
            <BehaviorTree ID="Untitled">
                <Sequence>
                    <AlwaysSuccess/>
                    <AlwaysFailure/>
                    <AlwaysSuccess/>
                </Sequence>
            </BehaviorTree>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // First tick: fails on the second child
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::FAILURE);

    // Second tick: should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::FAILURE);
}

// ****************************************************************************
//! \brief Test Selector node with success.
//! \details Verifies that Selector node returns SUCCESS as soon as a child succeeds.
//! \note Compare Selector node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, SelectorNodeSuccess)
{
    // Our implementation
    auto& sel = m_tree->setRoot<Selector>();
    sel.addChild<AlwaysFailure>();
    sel.addChild<AlwaysSuccess>();
    sel.addChild<AlwaysSuccess>(); // Shall never be executed
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <root BTCPP_format="4">
            <BehaviorTree ID="Untitled">
                <Fallback>
                    <AlwaysFailure/>
                    <AlwaysSuccess/>
                    <AlwaysSuccess/>
                </Fallback>
            </BehaviorTree>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // First tick: stops at the first success
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);

    // Second tick: should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);
}

#if 0
// ****************************************************************************
//! \brief Test ParallelSequence node
//! \details Verifies ParallelSequence node behavior with minimum success threshold
//! \note Compare ParallelSequence node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, ParallelSequenceSuccess)
{
    // Our implementation
    size_t minSuccess = 2;
    size_t minFail = 2;
    auto& parallel = m_tree->setRoot<ParallelSequence>(minSuccess, minFail);
    parallel.addChild<AlwaysSuccess>();
    parallel.addChild<AlwaysSuccess>();
    parallel.addChild<AlwaysFailure>();
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <ParallelAll threshold_success="2" threshold_failure="2">
                <AlwaysSuccess name="success1"/>
                <AlwaysSuccess name="success2"/>
                <AlwaysFailure name="failure"/>
            </ParallelAll>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // First tick: reached the success threshold
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);
}

// ****************************************************************************
//! \brief Test ParallelSequence node
//! \details Verifies ParallelSequence node behavior with minimum success threshold
//! \note Compare ParallelSequence node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, ParallelSequenceFailure)
{
    // Our implementation
    size_t minSuccess = 2;
    size_t minFail = 2;
    auto& parallel = m_tree->setRoot<ParallelSequence>(minSuccess, minFail);
    parallel.addChild<AlwaysFailure>();
    parallel.addChild<AlwaysFailure>();
    parallel.addChild<AlwaysSuccess>();
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <ParallelAll threshold_success="2" threshold_failure="2">
                <AlwaysFailure name="failure1"/>
                <AlwaysFailure name="failure2"/>
                <AlwaysSuccess name="success"/>
            </ParallelAll>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Should fail because failure threshold reached
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::FAILURE);
}
#endif

// ****************************************************************************
//! \brief Compare Inverter node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, InverterTestSuccess)
{
    // Our implementation
    auto& inv = m_tree->setRoot<Inverter>();
    inv.setChild<AlwaysFailure>();
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <root BTCPP_format="4">
            <BehaviorTree ID="Untitled">
                <Inverter>
                    <AlwaysFailure/>
                </Inverter>
            </BehaviorTree>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);
}

// ****************************************************************************
//! \brief Compare Inverter node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, InverterTestFailure)
{
    // Our implementation
    auto& inv = m_tree->setRoot<Inverter>();
    inv.setChild<AlwaysSuccess>();
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <root BTCPP_format="4">
            <BehaviorTree ID="Untitled">
                <Inverter>
                    <AlwaysSuccess/>
                </Inverter>
            </BehaviorTree>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::FAILURE);
}

// ****************************************************************************
//! \brief Test Repeat node.
//! \details Verifies that Repeat repeats its child's action the specified number of times.
//! \note Compare Repeat node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, RepeatTest)
{
    // Our implementation
    size_t num_cycles = 3;
    auto& rep = m_tree->setRoot<Repeat>(num_cycles);
    rep.setChild<AlwaysSuccess>();
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <root BTCPP_format="4">
        <BehaviorTree ID="Untitled">
            <Repeat num_cycles="3">
                <AlwaysSuccess/>
            </Repeat>
        </BehaviorTree>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results for each tick
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);

    EXPECT_EQ(rep.getCount(), num_cycles);
    EXPECT_EQ(rep.getRepetitions(), num_cycles);

    // BehaviorTree.CPP makes the loop within the tick time.
    //EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::RUNNING);
    //EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::RUNNING);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);
}

// ****************************************************************************
//! \brief Test Repeat node with a child that returns RUNNING.
//! \details Verifies that Repeat node returns RUNNING when its child returns RUNNING.
// ****************************************************************************
TEST_F(CompareBehaviorTest, RepeatWithRunningChild)
{
    // Create a mock action that returns RUNNING
    class RunningAction : public Leaf
    {
    public:
        virtual Status onRunning() override { return Status::RUNNING; }
    };

    // Our implementation
    size_t num_cycles = 3;
    auto& rep = m_tree->setRoot<Repeat>(num_cycles);
    rep.setChild<RunningAction>();
    EXPECT_EQ(m_tree->isValid(), true);

    // First tick: child returns RUNNING, so Repeat should return RUNNING
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    EXPECT_EQ(m_tree->tick(), Status::RUNNING);

    // Counter should not increment while child is RUNNING
    EXPECT_EQ(rep.getCount(), 0);
    EXPECT_EQ(rep.getRepetitions(), num_cycles);
}

// ****************************************************************************
//! \brief Test Repeat node with a child that returns FAILURE.
//! \details Verifies that Repeat node returns FAILURE when its child returns FAILURE.
// ****************************************************************************
TEST_F(CompareBehaviorTest, RepeatWithFailureChild)
{
    // Our implementation
    size_t num_cycles = 3;
    auto& rep = m_tree->setRoot<Repeat>(num_cycles);
    rep.setChild<AlwaysFailure>();
    EXPECT_EQ(m_tree->isValid(), true);

    // First tick: child returns FAILURE, so Repeat should return FAILURE
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);

    // Counter should not increment on FAILURE
    EXPECT_EQ(rep.getCount(), 0);
    EXPECT_EQ(rep.getRepetitions(), num_cycles);
}

#if 0
// ****************************************************************************
//! \brief Compare UntilSuccess node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareUntilSuccess)
{
    // Our implementation
    auto& until = m_tree->setRoot<UntilSuccess>();
    until.setChild<MockAction>(Status::FAILURE);
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <RetryUntilSuccessful num_attempts="-1">
                <AlwaysFailure/>
            </RetryUntilSuccessful>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickOnce());
}

// ****************************************************************************
//! \brief Compare UntilFailure node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareUntilFailure)
{
    // Our implementation
    auto& until = m_tree->setRoot<UntilFailure>();
    until.setChild<MockAction>(Status::SUCCESS);
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <RetryUntilFailure num_attempts="-1">
                <AlwaysSuccess/>
            </RetryUntilFailure>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickOnce());
}

// ****************************************************************************
//! \brief Test StatefulSelector node.
//! \details Verifies stateful behavior of Selector node.
// ****************************************************************************
TEST_F(CompareBehaviorTest, StatefulSelectorTest)
{
    // Our implementation
    auto& selector = m_tree->setRoot<StatefulSelector>();
    selector.addChild<MockAction>(Status::FAILURE);
    selector.addChild<MockAction>(Status::SUCCESS);
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);

    selector.reset();

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::SUCCESS);
}

// ****************************************************************************
//! \brief Test StatefulSequence node.
//! \details Verifies stateful behavior of Sequence node.
// ****************************************************************************
TEST_F(BehaviorTreeTest, StatefulSequenceTest)
{
    // Our implementation
    auto& seq = m_tree->setRoot<StatefulSequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::FAILURE);
    EXPECT_EQ(m_tree->isValid(), true);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::FAILURE);

    seq.reset();


    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    EXPECT_EQ(btcpp_tree.tickOnce(), BT::NodeStatus::FAILURE);
}
#endif

} // namespace bt