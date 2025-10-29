//*****************************************************************************
//! \brief Complete example using all nodes from the BehaviorTree library
//*****************************************************************************

#include <BehaviorTree/BehaviorTree.hpp>
#include <BehaviorTree/Builder.hpp>
#include <BehaviorTree/Visualizer.hpp>

#include <chrono>
#include <iostream>
#include <string>

namespace bt::demo {

//*****************************************************************************
//! \brief Example of a game character state
//*****************************************************************************
struct CharacterState
{
    bool isHungry = false;
    bool isTired = false;
    bool hasEnemyNearby = false;
    bool hasWeapon = false;
    bool hasAmmo = false;
    bool isSafeToFight = false;
    bool needsResources = false;
    bool hasTool = false;
    bool hasIngredients = false;
    bool isDangerous = false;
    int health = 100;
    int energy = 100;
    int resources = 0;
};

//*****************************************************************************
//! \brief Example of a game character
//*****************************************************************************
class Character
{
public:

    Character()
    {
        m_state = std::make_shared<CharacterState>();
    }

    // ------------------------------------------------------------------------
    //! \brief Actions
    // ------------------------------------------------------------------------
    Status eat() const
    {
        std::cout << "🍽️ Eating...\n";
        m_state->isHungry = false;
        m_state->energy = std::min(100, m_state->energy + 20);
        return Status::SUCCESS;
    }

    Status sleep() const
    {
        std::cout << "😴 Sleeping...\n";
        m_state->isTired = false;
        m_state->energy = 100;
        return Status::SUCCESS;
    }

    Status fight() const
    {
        std::cout << "⚔️ Fighting...\n";
        m_state->hasEnemyNearby = false;
        m_state->health = std::max(0, m_state->health - 10);
        return Status::SUCCESS;
    }

    Status heal() const
    {
        std::cout << "💊 Healing...\n";
        m_state->health = std::min(100, m_state->health + 20);
        return Status::SUCCESS;
    }

    Status gather() const
    {
        std::cout << "⛏️ Gathering resources...\n";
        m_state->resources += 10;
        m_state->needsResources = m_state->resources < 50;
        return Status::SUCCESS;
    }

    Status craft() const
    {
        std::cout << "🔨 Crafting...\n";
        m_state->hasIngredients = false;
        return Status::SUCCESS;
    }

    Status storeItem() const
    {
        std::cout << "📦 Storing item...\n";
        return Status::SUCCESS;
    }

    Status patrol() const
    {
        std::cout << "🚶 Patrolling...\n";
        return Status::SUCCESS;
    }

    Status idle() const
    {
        std::cout << "😐 Idling...\n";
        return Status::SUCCESS;
    }

    // ------------------------------------------------------------------------
    //! \brief Conditions
    // ------------------------------------------------------------------------
    Status isHungry() const
    {
        return m_state->isHungry ? Status::SUCCESS : Status::FAILURE;
    }

    Status isTired() const
    {
        return m_state->isTired ? Status::SUCCESS : Status::FAILURE;
    }

    Status hasEnemyNearby() const
    {
        return m_state->hasEnemyNearby ? Status::SUCCESS : Status::FAILURE;
    }

    Status isLowHealth() const
    {
        return m_state->health < 30 ? Status::SUCCESS : Status::FAILURE;
    }

    Status hasWeapon() const
    {
        return m_state->hasWeapon ? Status::SUCCESS : Status::FAILURE;
    }

    Status hasAmmo() const
    {
        return m_state->hasAmmo ? Status::SUCCESS : Status::FAILURE;
    }

    Status isSafeToFight() const
    {
        return m_state->isSafeToFight ? Status::SUCCESS : Status::FAILURE;
    }

    Status needsResources() const
    {
        return m_state->needsResources ? Status::SUCCESS : Status::FAILURE;
    }

    Status hasTool() const
    {
        return m_state->hasTool ? Status::SUCCESS : Status::FAILURE;
    }

    Status hasIngredients() const
    {
        return m_state->hasIngredients ? Status::SUCCESS : Status::FAILURE;
    }

    Status isDangerous() const
    {
        return m_state->isDangerous ? Status::SUCCESS : Status::FAILURE;
    }

    // ------------------------------------------------------------------------
    //! \brief State updates
    // ------------------------------------------------------------------------
    void update() const
    {
        // Randomly update state
        m_state->isHungry = rand() % 100 < 20;
        m_state->isTired = rand() % 100 < 30;
        m_state->hasEnemyNearby = rand() % 100 < 10;
        m_state->hasWeapon = rand() % 100 < 70;
        m_state->hasAmmo = rand() % 100 < 60;
        m_state->isSafeToFight = rand() % 100 < 80;
        m_state->needsResources = m_state->resources < 50;
        m_state->hasTool = rand() % 100 < 90;
        m_state->hasIngredients = rand() % 100 < 40;
        m_state->isDangerous = rand() % 100 < 15;
        m_state->energy = std::max(0, m_state->energy - 5);
    }

    // ------------------------------------------------------------------------
    //! \brief Getters
    // ------------------------------------------------------------------------
    int getHealth() const
    {
        return m_state->health;
    }
    int getEnergy() const
    {
        return m_state->energy;
    }
    int getResources() const
    {
        return m_state->resources;
    }
    bool getIsHungry() const
    {
        return m_state->isHungry;
    }
    bool getIsTired() const
    {
        return m_state->isTired;
    }
    bool getHasEnemyNearby() const
    {
        return m_state->hasEnemyNearby;
    }
    bool getHasWeapon() const
    {
        return m_state->hasWeapon;
    }
    bool getHasAmmo() const
    {
        return m_state->hasAmmo;
    }
    bool getIsSafeToFight() const
    {
        return m_state->isSafeToFight;
    }
    bool getNeedsResources() const
    {
        return m_state->needsResources;
    }
    bool getHasTool() const
    {
        return m_state->hasTool;
    }
    bool getHasIngredients() const
    {
        return m_state->hasIngredients;
    }
    bool getIsDangerous() const
    {
        return m_state->isDangerous;
    }

private:

    std::shared_ptr<CharacterState> m_state;
};

//*****************************************************************************
//! \brief Factory to register custom actions
//*****************************************************************************
class CharacterFactory: public bt::NodeFactory
{
public:

    CharacterFactory(bt::Blackboard::Ptr p_blackboard,
                     Character const* p_character)
    {
        // Register actions
        registerAction(
            "eat",
            [p_blackboard, p_character]() noexcept {
                return p_character->eat();
            },
            p_blackboard);
        registerAction(
            "sleep",
            [p_blackboard, p_character]() noexcept {
                return p_character->sleep();
            },
            p_blackboard);
        registerAction(
            "fight",
            [p_blackboard, p_character]() noexcept {
                return p_character->fight();
            },
            p_blackboard);
        registerAction(
            "heal",
            [p_blackboard, p_character]() noexcept {
                return p_character->heal();
            },
            p_blackboard);
        registerAction(
            "gather",
            [p_blackboard, p_character]() noexcept {
                return p_character->gather();
            },
            p_blackboard);
        registerAction(
            "craft",
            [p_blackboard, p_character]() noexcept {
                return p_character->craft();
            },
            p_blackboard);
        registerAction(
            "store_item",
            [p_blackboard, p_character]() noexcept {
                return p_character->storeItem();
            },
            p_blackboard);
        registerAction(
            "patrol",
            [p_blackboard, p_character]() noexcept {
                return p_character->patrol();
            },
            p_blackboard);
        registerAction(
            "idle",
            [p_blackboard, p_character]() noexcept {
                return p_character->idle();
            },
            p_blackboard);

        // Register conditions
        registerAction(
            "is_hungry",
            [p_blackboard, p_character]() noexcept {
                return p_character->isHungry();
            },
            p_blackboard);
        registerAction(
            "is_tired",
            [p_blackboard, p_character]() noexcept {
                return p_character->isTired();
            },
            p_blackboard);
        registerAction(
            "has_enemy",
            [p_blackboard, p_character]() noexcept {
                return p_character->hasEnemyNearby();
            },
            p_blackboard);
        registerAction(
            "is_low_health",
            [p_blackboard, p_character]() noexcept {
                return p_character->isLowHealth();
            },
            p_blackboard);
        registerAction(
            "has_weapon",
            [p_blackboard, p_character]() noexcept {
                return p_character->hasWeapon();
            },
            p_blackboard);
        registerAction(
            "has_ammo",
            [p_blackboard, p_character]() noexcept {
                return p_character->hasAmmo();
            },
            p_blackboard);
        registerAction(
            "is_safe_to_fight",
            [p_blackboard, p_character]() noexcept {
                return p_character->isSafeToFight();
            },
            p_blackboard);
        registerAction(
            "needs_resources",
            [p_blackboard, p_character]() noexcept {
                return p_character->needsResources();
            },
            p_blackboard);
        registerAction(
            "has_tool",
            [p_blackboard, p_character]() noexcept {
                return p_character->hasTool();
            },
            p_blackboard);
        registerAction(
            "has_ingredients",
            [p_blackboard, p_character]() noexcept {
                return p_character->hasIngredients();
            },
            p_blackboard);
        registerAction(
            "is_dangerous",
            [p_blackboard, p_character]() noexcept {
                return p_character->isDangerous();
            },
            p_blackboard);
    }
};

//*****************************************************************************
//! \brief Run the complete example demo
//*****************************************************************************
void runDemo()
{
    // Create character and blackboard
    Character character;
    auto blackboard = std::make_shared<Blackboard>();
    blackboard->set("character", &character);

    // Create the factory with custom actions
    CharacterFactory factory(blackboard, &character);

    // Load the tree from the YAML file
    auto tree = bt::Builder::fromFile(
        factory, "doc/demos/complete_example/complete_example.yaml");
    if (!tree)
    {
        std::cerr << "Failed to load behavior tree from YAML\n";
        return;
    }

    // Initialize the connection with the visualizer
    Visualizer visualizer(*tree);
    if (auto ec =
            visualizer.connect("127.0.0.1", 9090, std::chrono::seconds(5)))
    {
        std::cerr << "Failed to connect to the visualizer: " << ec.message()
                  << std::endl;
        return;
    }

    // Main loop
    std::cout << "Starting complete example demo...\n";
    for (int i = 0; i < 20; ++i)
    {
        std::cout << "\n--- Tick " << i << " ---\n";
        std::cout << "Health: " << character.getHealth()
                  << "% | Energy: " << character.getEnergy()
                  << "% | Resources: " << character.getResources() << "\n";

        character.update();
        if (tree->tick() != Status::RUNNING)
        {
            std::cout << "Tree is finished\n";
        }

        // Update the visualizer
        visualizer.tick();

        // Wait for the next tick
        std::string input;
        std::cout << "Press Enter to continue..." << std::endl;
        std::getline(std::cin, input);
    }

    visualizer.disconnect();
}

} // namespace bt::demo

//*****************************************************************************
//! \brief Main function
//*****************************************************************************
int main()
{
    try
    {
        bt::demo::runDemo();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}