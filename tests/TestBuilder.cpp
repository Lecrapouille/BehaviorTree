#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "BehaviorTree/TreeBuilder.hpp"
#include <fstream>  // Pour std::ofstream
#include <sstream>  // Pour std::stringstream

namespace bt {

// ****************************************************************************
//! \brief Test fixture for TreeBuilder class
//! \details This class provides the test environment for the TreeBuilder functionality,
//! including YAML and XML parsing capabilities.
// ****************************************************************************
class TreeBuilderTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Setup code
    }
};

// ****************************************************************************
//! \brief Test loading a simple behavior tree from YAML format
//! \details Tests the creation of a basic behavior tree with a sequence node
//! containing an action and a condition node.
// ****************************************************************************
TEST_F(TreeBuilderTest, LoadSimpleYAMLTree)
{
    std::string yaml_content = R"(
        behavior_tree:
          type: sequence
          children:
            - type: action
              name: move
            - type: condition
              name: is_target_reached
    )";

    // Write temporary file
    std::ofstream temp("test.yaml");
    temp << yaml_content;
    temp.close();

    auto tree = bt::TreeBuilder::fromYAML("test.yaml");
    ASSERT_NE(tree, nullptr);
    EXPECT_TRUE(dynamic_cast<bt::Sequence*>(&tree->root()) != nullptr);

    // Cleanup
    std::remove("test.yaml");
}

// ****************************************************************************
//! \brief Test loading a behavior tree from BehaviorTree.CPP XML format
//! \details Verifies the ability to parse and create a tree from the standard
//! BehaviorTree.CPP XML format (version 4).
// ****************************************************************************
TEST_F(TreeBuilderTest, LoadXMLFromBehaviorTreeCPP)
{
    std::string xml_content = R"(
        <root BTCPP_format="4">
            <BehaviorTree ID="MainTree">
                <Sequence name="root">
                    <Action name="MoveBase"/>
                    <Condition name="IsTargetReached"/>
                </Sequence>
            </BehaviorTree>
        </root>
    )";

    // Write temporary file
    std::ofstream temp("test.xml");
    temp << xml_content;
    temp.close();

    auto tree = bt::TreeBuilder::fromXML("test.xml");
    ASSERT_NE(tree, nullptr);

    // Cleanup
    std::remove("test.xml");
}

// ****************************************************************************
//! \brief Test loading a complex behavior tree from YAML
//! \details Tests the creation of a more complex tree structure with nested
//! sequence and selector nodes. Verifies proper node hierarchy
//! and child relationships.
// ****************************************************************************
TEST(TreeBuilder, LoadYAML)
{
    // Create a temporary YAML file for the test
    std::ofstream temp("test.yaml");
    temp << R"(
behavior_tree:
  type: sequence
  children:
    - type: selector
      children:
        - type: sequence
    - type: selector
)";
    temp.close();

    // Load and verify the tree
    auto tree = bt::TreeBuilder::fromYAML("test.yaml");
    ASSERT_TRUE(tree != nullptr);
    ASSERT_TRUE(tree->getRoot() != nullptr);

    // Verify that the structure is correct
    auto root = dynamic_cast<bt::Sequence*>(tree->getRoot().get());
    ASSERT_TRUE(root != nullptr);
    ASSERT_EQ(root->children().size(), 2);

    auto firstChild = dynamic_cast<bt::Selector*>(root->children()[0].get());
    ASSERT_TRUE(firstChild != nullptr);
    ASSERT_EQ(firstChild->children().size(), 1);

    auto secondChild = dynamic_cast<bt::Selector*>(root->children()[1].get());
    ASSERT_TRUE(secondChild != nullptr);
    ASSERT_EQ(secondChild->children().size(), 0);

    // Clean up
    std::remove("test.yaml");
}

} // namespace bt