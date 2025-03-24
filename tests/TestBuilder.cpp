#include "BehaviorTree/Builder.hpp"
#include "BehaviorTree/Exporter.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

namespace bt {

// ****************************************************************************
//! \brief Fixture pour les tests de Builder
// ****************************************************************************
class BuilderTest: public ::testing::Test
{
protected:

    void SetUp() override
    {
        yaml_tree = R"(behavior_tree:
  Selector:
    children:
      - Sequence:
          children:
            - Condition:
                name: IsEnemyVisible
            - Retry:
                attempts: 3
                child:
                  - Action:
                      name: AttackEnemy
      - Sequence:
          children:
            - Condition:
                name: IsAtPatrolPoint
            - Repeat:
                times: 2
                child:
                  - Action:
                      name: MoveToNextPatrolPoint
      - Inverter:
          child:
            - Condition:
                name: IsLowHealth
      - Action:
          name: Idle)";

        factory = std::make_shared<NodeFactory>();
        factory->registerCondition("IsEnemyVisible", []() { return true; });
        factory->registerCondition("IsAtPatrolPoint", []() { return false; });
        factory->registerCondition("IsLowHealth", []() { return true; });
        factory->registerAction("AttackEnemy",
                                []() { return Status::SUCCESS; });
        factory->registerAction("MoveToNextPatrolPoint",
                                []() { return Status::FAILURE; });
        factory->registerAction("Idle", []() { return Status::RUNNING; });

        builder = std::make_unique<Builder>(factory);
        tree = builder->fromText(yaml_tree);
    }

protected:

    std::string yaml_tree;
    std::shared_ptr<NodeFactory> factory;
    std::unique_ptr<Builder> builder;
    std::unique_ptr<Tree> tree;
};

// ****************************************************************************
//! \brief Test loading a simple behavior tree from YAML format
//! \details Tests the creation of a basic behavior tree with a sequence node
//! containing an action and a condition node.
// ****************************************************************************
TEST_F(BuilderTest, LoadSimpleYAMLTree)
{
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->isValid(), true);

    // Check the root node (Selector)
    auto root = dynamic_cast<bt::Selector*>(tree->getRoot().get());
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getChildren().size(), 4u);

    auto const& root_children = root->getChildren();

    // First branch: Sequence with IsEnemyVisible and Retry(AttackEnemy)
    auto first_sequence = dynamic_cast<bt::Sequence*>(root_children[0].get());
    ASSERT_NE(first_sequence, nullptr);
    EXPECT_EQ(first_sequence->getChildren().size(), 2u);

    auto is_enemy_visible =
        dynamic_cast<bt::Action*>(first_sequence->getChildren()[0].get());
    ASSERT_NE(is_enemy_visible, nullptr);
    EXPECT_EQ(is_enemy_visible->name, "IsEnemyVisible");

    auto retry_node =
        dynamic_cast<bt::Retry*>(first_sequence->getChildren()[1].get());
    ASSERT_NE(retry_node, nullptr);
    EXPECT_EQ(retry_node->getAttempts(), 3u);

    auto attack_enemy = dynamic_cast<bt::Action*>(retry_node->getChild().get());
    ASSERT_NE(attack_enemy, nullptr);
    EXPECT_EQ(attack_enemy->name, "AttackEnemy");

    // Second branch: Sequence with IsAtPatrolPoint and
    // Repeat(MoveToNextPatrolPoint)
    auto second_sequence = dynamic_cast<bt::Sequence*>(root_children[1].get());
    ASSERT_NE(second_sequence, nullptr);
    EXPECT_EQ(second_sequence->getChildren().size(), 2u);

    auto is_at_patrol =
        dynamic_cast<bt::Action*>(second_sequence->getChildren()[0].get());
    ASSERT_NE(is_at_patrol, nullptr);
    EXPECT_EQ(is_at_patrol->name, "IsAtPatrolPoint");

    auto repeat_node =
        dynamic_cast<bt::Repeat*>(second_sequence->getChildren()[1].get());
    ASSERT_NE(repeat_node, nullptr);
    EXPECT_EQ(repeat_node->getRepetitions(), 2u);

    auto move_to_patrol =
        dynamic_cast<bt::Action*>(repeat_node->getChild().get());
    ASSERT_NE(move_to_patrol, nullptr);
    EXPECT_EQ(move_to_patrol->name, "MoveToNextPatrolPoint");

    // Third branch: Inverter(IsLowHealth)
    auto inverter = dynamic_cast<bt::Inverter*>(root_children[2].get());
    ASSERT_NE(inverter, nullptr);

    auto is_low_health = dynamic_cast<bt::Action*>(inverter->getChild().get());
    ASSERT_NE(is_low_health, nullptr);
    EXPECT_EQ(is_low_health->name, "IsLowHealth");

    // Fourth branch: Action(Idle)
    auto idle = dynamic_cast<bt::Action*>(root_children[3].get());
    ASSERT_NE(idle, nullptr);
    EXPECT_EQ(idle->name, "Idle");

    // Test of the YAML export
    auto yaml_content = bt::Exporter::toYAML(*tree);
    EXPECT_EQ(yaml_content, yaml_tree);

    EXPECT_EQ(bt::Exporter::toYAMLFile(*tree, "/tmp/test.yaml"), true);
    std::ifstream file("/tmp/test.yaml");
    ASSERT_TRUE(file.is_open()) << "Impossible to open the YAML file";
    std::stringstream buffer;
    buffer << file.rdbuf();
    EXPECT_EQ(buffer.str(), yaml_tree);
    std::remove("/tmp/test.yaml");
}

// ****************************************************************************
//! \brief Test the generation of a Mermaid diagram from a behavior tree
//! \details Verifies that the toMermaid method generates a correct Mermaid
//! diagram representing the behavior tree.
// ****************************************************************************
TEST_F(BuilderTest, TestToMermaid)
{
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->isValid(), true);

    // Generate the Mermaid diagram
    std::string mermaid = builder->toMermaid(*tree);

    // Verify that the diagram contains the Mermaid header
    EXPECT_TRUE(mermaid.find("graph TD") != std::string::npos);

    // Verify that all nodes are present in the diagram
    EXPECT_TRUE(mermaid.find("node1") != std::string::npos); // Root node

    // Verify the connections between nodes
    // Note: The exact IDs depend on the implementation of generateMermaidNode
    // We therefore simply check the presence of connections
    EXPECT_TRUE(mermaid.find("-->") != std::string::npos);

    // Verify that the diagram can be written to a file
    std::string filename = "/tmp/test_mermaid.md";
    std::ofstream file(filename);
    ASSERT_TRUE(file.is_open()) << "Impossible to open the Mermaid file";
    file << mermaid;
    file.close();

    // Read the file and verify its content
    std::ifstream infile(filename);
    ASSERT_TRUE(infile.is_open()) << "Impossible to read the Mermaid file";
    std::stringstream buffer;
    buffer << infile.rdbuf();
    EXPECT_EQ(buffer.str(), mermaid);

    // Clean up
    std::remove(filename.c_str());
}

} // namespace bt