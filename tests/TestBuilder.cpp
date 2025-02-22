#include "BehaviorTree/TreeBuilder.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream> 
#include <sstream>

namespace bt {

// ****************************************************************************
//! \brief Test loading a simple behavior tree from YAML format
//! \details Tests the creation of a basic behavior tree with a sequence node
//! containing an action and a condition node.
// ****************************************************************************
TEST(TreeBuilder, LoadSimpleYAMLTree)
{
    std::string yaml_content = R"(
        behavior_tree:
          type: sequence
          name: test_sequence
          children:
            - type: always_success
              name: move
            - type: always_success
              name: is_target_reached
    )";

    // Write temporary file
    std::ofstream temp("test.yaml");
    temp << yaml_content;
    temp.close();

    auto factory = std::make_shared<NodeFactory>();
    // Register basic node types
    factory->registerNodeType<AlwaysSuccess>("always_success"); 
    factory->registerNodeType<Sequence>("sequence");
    
    TreeBuilder builder(factory);
    auto tree = builder.fromYAML("test.yaml");
    
    ASSERT_NE(tree, nullptr);
    auto root = dynamic_cast<bt::Sequence*>(tree->rootNode());
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->name(), "test_sequence");
    EXPECT_EQ(root->children().size(), 2);

    // Cleanup
    std::remove("test.yaml");
}

// ****************************************************************************
//! \brief Test loading a behavior tree from BehaviorTree.CPP XML format
//! \details Verifies the ability to parse and create a tree from the standard
//! BehaviorTree.CPP XML format (version 4).
// ****************************************************************************
TEST(TreeBuilder, LoadXMLFromBehaviorTreeCPP)
{
    // TODO: Implement XML support
    GTEST_SKIP() << "XML support not yet implemented";
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
    auto factory = std::make_shared<NodeFactory>();
    TreeBuilder builder(factory);
    auto tree = builder.fromYAML("test.yaml");
    ASSERT_TRUE(tree != nullptr);
    ASSERT_TRUE(tree->getRoot() != nullptr);

    // Verify that the structure is correct
    auto root = dynamic_cast<bt::Sequence*>(tree->getRoot().get());
    ASSERT_TRUE(root != nullptr);
    ASSERT_EQ(root->getChildren().size(), 2);

    auto firstChild = dynamic_cast<bt::Selector*>(root->getChildren()[0].get());
    ASSERT_TRUE(firstChild != nullptr);
    ASSERT_EQ(firstChild->getChildren().size(), 1);

    auto secondChild = dynamic_cast<bt::Selector*>(root->getChildren()[1].get());
    ASSERT_TRUE(secondChild != nullptr);
    ASSERT_EQ(secondChild->getChildren().size(), 0);

    // Clean up
    std::remove("test.yaml");
}

// ****************************************************************************
//! \brief Test handling of invalid YAML input
//! \details Verifies that the builder properly handles and reports errors
//! when given invalid YAML input.
// ****************************************************************************
TEST(TreeBuilder, HandleInvalidYAML) 
{
    std::string invalid_yaml = R"(
        behavior_tree:
          type: invalid_node
          name: test_invalid
          children:
            - type: action
              name: test_action
    )";

    std::ofstream temp("invalid.yaml");
    temp << invalid_yaml;
    temp.close();

    auto factory = std::make_shared<NodeFactory>();
    TreeBuilder builder(factory);

    EXPECT_THROW({
        try {
            builder.fromYAML("invalid.yaml");
        }
        catch(const RuntimeError& e) {
            // Vérifie que l'erreur contient des informations utiles
            EXPECT_THAT(e.what(), testing::HasSubstr("invalid_node"));
            throw;
        }
    }, RuntimeError);

    std::remove("invalid.yaml");
}

} // namespace bt