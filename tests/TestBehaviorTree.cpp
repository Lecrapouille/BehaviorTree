//*****************************************************************************
// Unit tests for BehaviorTree
//*****************************************************************************

#include "BehaviorTree/TreeBuilder.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include "BehaviorTree/TreeExporter.hpp"

namespace bt {

//! \brief Mock action node for testing
class MockAction : public Leaf
{
public:
    //! \brief Constructor
    //! \param[in] result Status to return on tick
    explicit MockAction(Status result)
        : m_result(result)
    {}

    virtual Status onRunning() override
    {
        return m_result;
    }

private:
    Status m_result;
};

//! \brief Test fixture for BehaviorTree tests
class BehaviorTreeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_tree = std::make_unique<BehaviorTree>();
    }

    //! \brief Helper to create a temporary YAML file
    //! \param[in] content YAML content
    //! \return Path to created file
    std::string createTempYAML(std::string const& content)
    {
        std::string filename = "/tmp/temp_test.yaml";
        std::ofstream file(filename);
        file << content;
        file.close();
        return filename;
    }

    std::unique_ptr<BehaviorTree> m_tree;
};

TEST_F(BehaviorTreeTest, SequenceNodeSuccess)
{
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

TEST_F(BehaviorTreeTest, SequenceNodeFailure)
{
    auto& seq = m_tree->setRoot<Sequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::FAILURE);

    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
}

TEST_F(BehaviorTreeTest, SelectorNodeSuccess)
{
    auto& sel = m_tree->setRoot<Selector>();
    sel.addChild<MockAction>(Status::FAILURE);
    sel.addChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

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

TEST_F(BehaviorTreeTest, InverterTest)
{
    auto& inv = m_tree->setRoot<Inverter>();
    inv.setChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
}

TEST_F(BehaviorTreeTest, RepeaterTest)
{
    size_t limit = 2;
    auto& rep = m_tree->setRoot<Repeater>(limit);
    rep.setChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::RUNNING);
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

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

TEST_F(BehaviorTreeTest, YAMLInvalidFile)
{
    EXPECT_THROW(TreeBuilder::fromYAML("nonexistent.yaml"), std::runtime_error);
}

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

TEST_F(BehaviorTreeTest, UntilSuccessTest)
{
    auto& until = m_tree->setRoot<UntilSuccess>();
    auto& seq = until.setChild<Sequence>();
    seq.addChild<MockAction>(Status::FAILURE);
    seq.addChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

TEST_F(BehaviorTreeTest, UntilFailureTest)
{
    auto& until = m_tree->setRoot<UntilFailure>();
    auto& seq = until.setChild<Sequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::FAILURE);

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

TEST_F(BehaviorTreeTest, StatefulSelectorTest)
{
    auto& selector = m_tree->setRoot<StatefulSelector>();
    selector.addChild<MockAction>(Status::FAILURE);
    selector.addChild<MockAction>(Status::SUCCESS);

    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
    selector.reset();
    EXPECT_EQ(m_tree->tick(), Status::SUCCESS);
}

TEST_F(BehaviorTreeTest, StatefulSequenceTest)
{
    auto& seq = m_tree->setRoot<StatefulSequence>();
    seq.addChild<MockAction>(Status::SUCCESS);
    seq.addChild<MockAction>(Status::FAILURE);

    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
    seq.reset();
    EXPECT_EQ(m_tree->tick(), Status::FAILURE);
}

//! \brief Test fixture for Blackboard tests
class BlackboardTest : public ::testing::Test
{
protected:
    bt::Blackboard::Ptr blackboard;

    void SetUp() override {
        blackboard = std::make_shared<bt::Blackboard>();
    }
};

//! \brief Test basic set/get operations
TEST_F(BlackboardTest, BasicSetGet)
{
    // Test with different types
    blackboard->set<int>("int_val", 42);
    blackboard->set<std::string>("str_val", "hello");
    blackboard->set<double>("double_val", 3.14);

    auto [int_val, int_found] = blackboard->get<int>("int_val");
    EXPECT_TRUE(int_found);
    EXPECT_EQ(int_val, 42);

    auto [str_val, str_found] = blackboard->get<std::string>("str_val");
    EXPECT_TRUE(str_found);
    EXPECT_EQ(str_val, "hello");

    auto [double_val, double_found] = blackboard->get<double>("double_val");
    EXPECT_TRUE(double_found);
    EXPECT_EQ(double_val, 3.14);
}

//! \brief Test empty key handling
TEST_F(BlackboardTest, EmptyKey)
{
    EXPECT_THROW(blackboard->set<int>("", 42), std::invalid_argument);
}

//! \brief Test non-existent key handling
TEST_F(BlackboardTest, NonExistentKey)
{
    auto result = blackboard->get<int>("non_existent");
    EXPECT_FALSE(result.second);
    EXPECT_EQ(blackboard->getOr<int>("non_existent", 100), 100);
}

//! \brief Test has() method
TEST_F(BlackboardTest, HasKey)
{
    blackboard->set<int>("test_key", 42);

    EXPECT_TRUE(blackboard->has<int>("test_key"));
    EXPECT_FALSE(blackboard->has<int>("non_existent"));
    EXPECT_FALSE(blackboard->has<std::string>("test_key")); // Different type
}

//! \brief Test remove() method
TEST_F(BlackboardTest, Remove)
{
    blackboard->set<int>("test_key", 42);

    EXPECT_TRUE(blackboard->remove<int>("test_key"));
    EXPECT_FALSE(blackboard->has<int>("test_key"));
    EXPECT_FALSE(blackboard->remove<int>("test_key")); // Already removed
    EXPECT_FALSE(blackboard->remove<int>("non_existent"));
}

//! \brief Test getKeys() method
TEST_F(BlackboardTest, GetKeys)
{
    blackboard->set<int>("key1", 1);
    blackboard->set<int>("key2", 2);
    blackboard->set<std::string>("str_key", "hello");

    auto int_keys = blackboard->getKeys<int>();
    auto str_keys = blackboard->getKeys<std::string>();

    EXPECT_EQ(int_keys.size(), 2);
    EXPECT_EQ(str_keys.size(), 1);

    // Check if keys are present (order may vary due to unordered_map)
    EXPECT_NE(std::find(int_keys.begin(), int_keys.end(), "key1"), int_keys.end());
    EXPECT_NE(std::find(int_keys.begin(), int_keys.end(), "key2"), int_keys.end());
    EXPECT_EQ(str_keys[0], "str_key");
}

//! \brief Test type safety
TEST_F(BlackboardTest, TypeSafety)
{
    blackboard->set<int>("value", 42);

    auto [double_val, double_found] = blackboard->get<double>("value");
    EXPECT_FALSE(double_found);

    auto [str_val, str_found] = blackboard->get<std::string>("value");
    EXPECT_FALSE(str_found);
}

//! \brief Test value overwriting
TEST_F(BlackboardTest, ValueOverwriting)
{
    blackboard->set<int>("test_key", 42);
    blackboard->set<int>("test_key", 100);

    auto [value, found] = blackboard->get<int>("test_key");
    EXPECT_TRUE(found);
    EXPECT_EQ(value, 100);
}

//! \brief Test complex types
TEST_F(BlackboardTest, ComplexTypes)
{
    struct TestStruct {
        int x;
        std::string y;
        bool operator==(const TestStruct& other) const {
            return x == other.x && y == other.y;
        }
    };

    TestStruct test_data{42, "hello"};
    blackboard->set<TestStruct>("complex", test_data);

    auto [retrieved, found] = blackboard->get<TestStruct>("complex");
    EXPECT_TRUE(found);
    EXPECT_EQ(retrieved, test_data);
}

//! \brief Test multiple blackboards independence
TEST_F(BlackboardTest, MultipleBlackboards)
{
    auto blackboard2 = std::make_shared<bt::Blackboard>();

    blackboard->set<int>("key", 42);
    blackboard2->set<int>("key", 100);

    auto [value1, found1] = blackboard->get<int>("key");
    EXPECT_TRUE(found1);
    EXPECT_EQ(value1, 42);

    auto [value2, found2] = blackboard2->get<int>("key");
    EXPECT_TRUE(found2);
    EXPECT_EQ(value2, 100);
}

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

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}