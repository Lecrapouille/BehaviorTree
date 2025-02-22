//*****************************************************************************
// Unit tests comparing with BehaviorTree.CPP
//*****************************************************************************

#include "BehaviorTree/BehaviorTree.hpp"
#include <behaviortree_cpp/behavior_tree.h>
#include <behaviortree_cpp/bt_factory.h>
#include <gtest/gtest.h>

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

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <Sequence name="sequence">
                <AlwaysSuccess name="success"/>
                <AlwaysFailure name="failure"/>
            </Sequence>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // First tick: all children succeed
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickRoot(), BT::NodeStatus::SUCCESS);
    
    // Second tick: should restart from the beginning
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    EXPECT_EQ(btcpp_tree.tickRoot(), BT::NodeStatus::SUCCESS);
}

// ****************************************************************************
//! \brief Compare Selector node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareSelectorNode)
{
    // Our implementation
    auto& sel = m_tree->setRoot<Selector>();
    sel.addChild<MockAction>(Status::FAILURE);
    sel.addChild<MockAction>(Status::SUCCESS);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <Fallback name="selector">
                <AlwaysFailure name="failure"/>
                <AlwaysSuccess name="success"/>
            </Fallback>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickRoot());
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickRoot());
}

// ****************************************************************************
//! \brief Compare ParallelSequence node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareParallelSequence)
{
    // Our implementation
    auto& parallel = m_tree->setRoot<ParallelSequence>(2, 2);
    parallel.addChild<MockAction>(Status::SUCCESS);
    parallel.addChild<MockAction>(Status::SUCCESS);
    parallel.addChild<MockAction>(Status::FAILURE);

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

    // Compare results
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickRoot());
}

// ****************************************************************************
//! \brief Compare Inverter node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareInverterNode)
{
    // Our implementation
    auto& inv = m_tree->setRoot<Inverter>();
    inv.setChild<MockAction>(Status::SUCCESS);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <Inverter>
                <AlwaysSuccess/>
            </Inverter>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickRoot());
    EXPECT_EQ(Status::FAILURE, btcpp_tree.tickRoot());
}

// ****************************************************************************
//! \brief Compare Repeater node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareRepeaterNode)
{
    // Our implementation
    size_t limit = 3;
    auto& rep = m_tree->setRoot<Repeater>(limit);
    rep.setChild<MockAction>(Status::SUCCESS);

    // BehaviorTree.CPP
    BT::BehaviorTreeFactory factory;
    std::string xml_text = R"(
        <root>
            <RetryUntilSuccessful num_attempts="3">
                <AlwaysSuccess/>
            </RetryUntilSuccessful>
        </root>)";
    auto btcpp_tree = factory.createTreeFromText(xml_text);

    // Compare results for each tick
    for (size_t i = 0; i < limit; ++i) {
        EXPECT_EQ(m_tree->tick(), btcpp_tree.tickRoot());
    }
}

// ****************************************************************************
//! \brief Compare UntilSuccess node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareUntilSuccess)
{
    // Our implementation
    auto& until = m_tree->setRoot<UntilSuccess>();
    until.setChild<MockAction>(Status::FAILURE);

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
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickRoot());
}

// ****************************************************************************
//! \brief Compare UntilFailure node behavior with BehaviorTree.CPP.
// ****************************************************************************
TEST_F(CompareBehaviorTest, CompareUntilFailure)
{
    // Our implementation
    auto& until = m_tree->setRoot<UntilFailure>();
    until.setChild<MockAction>(Status::SUCCESS);

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
    EXPECT_EQ(m_tree->tick(), btcpp_tree.tickRoot());
}

} // namespace bt