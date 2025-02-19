//*****************************************************************************
// Unit tests for BehaviorTree
//*****************************************************************************

#include "BehaviorTree/TreeBuilder.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include "BehaviorTree/TreeExporter.hpp"

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
        m_tree = std::make_unique<BehaviorTree>();
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

    std::unique_ptr<BehaviorTree> m_tree;
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

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
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
    - type: MockAction
      name: task1
      result: success
    - type: selector
      children:
        - type: MockAction
          name: task2
          result: failure
        - type: MockAction
          name: task3
          result: success
)";

    std::string filename = createTempYAML(yaml);
    auto tree = TreeBuilder::fromYAML(filename);
    ASSERT_NE(tree, nullptr);
    std::remove(filename.c_str());
}

// ****************************************************************************
//! \brief Test loading from invalid YAML file.
//! \details Verifies that appropriate exception is thrown when loading non-existent file.
// ****************************************************************************
TEST_F(BehaviorTreeTest, YAMLInvalidFile)
{
    EXPECT_THROW(TreeBuilder::fromYAML("nonexistent.yaml"), std::runtime_error);
}

// ****************************************************************************
//! \brief Test basic Blackboard operations.
//! \details Verifies read/write operations on different data types.
// ****************************************************************************
TEST_F(BehaviorTreeTest, BlackboardTest)
{
    auto blackboard = std::make_shared<Blackboard>();
    blackboard->set<int>("counter", 42);
    blackboard->set<std::string>("status", "running");

    auto [counter_val, counter_found] = blackboard->get<int>("counter");
    EXPECT_TRUE(counter_found);
    EXPECT_EQ(counter_val, 42);

    auto [status_val, status_found] = blackboard->get<std::string>("status");
    EXPECT_TRUE(status_found);
    EXPECT_EQ(status_val, "running");
}

// ****************************************************************************
//! \brief Test UntilSuccess node.
//! \details Verifies that node continues until success is achieved.
// ****************************************************************************
TEST_F(BehaviorTreeTest, UntilSuccessTest)
{
    auto& until = m_tree->setRoot<UntilSuccess>();
    auto& seq = until.setChild<Sequence>();
    seq.addChild<MockAction>(Status::FAILURE);
    seq.addChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

// ****************************************************************************
//! \brief Test UntilFailure node.
//! \details Verifies that node continues until failure is achieved.
// ****************************************************************************
TEST_F(BehaviorTreeTest, UntilFailureTest)
{
    auto& until = m_tree->setRoot<UntilFailure>();
    auto& seq = until.setChild<Sequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::FAILURE);

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
    // Create a sample tree
    auto& root = m_tree->setRoot<Sequence>();
    auto& selector = root.addChild<Selector>();
    selector.addChild<MockAction>(Status::SUCCESS);
    selector.addChild<MockAction>(Status::FAILURE);
    root.addChild<MockAction>(Status::SUCCESS);

    // Test YAML export
    std::string yaml = TreeExporter::toYAML(*m_tree);
    EXPECT_NE(yaml.find("behavior_tree:"), std::string::npos);
    EXPECT_NE(yaml.find("type: sequence"), std::string::npos);

    std::string yamlFile = "/tmp/test_tree.yaml";
    EXPECT_NO_THROW(TreeExporter::toYAMLFile(*m_tree, yamlFile));
    std::remove(yamlFile.c_str());

    // Test BehaviorTree.CPP XML export
    std::string xml = TreeExporter::toBTCppXML(*m_tree);
    EXPECT_NE(xml.find("<?xml version=\"1.0\" ?>"), std::string::npos);
    EXPECT_NE(xml.find("<BehaviorTree"), std::string::npos);

    std::string xmlFile = "/tmp/test_tree.xml";
    EXPECT_NO_THROW(TreeExporter::toBTCppXMLFile(*m_tree, xmlFile));
    std::remove(xmlFile.c_str());
}

} // namespace bt