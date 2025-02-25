//*****************************************************************************
// Security Robot Demo - Shows various behavior tree nodes in action
//*****************************************************************************

#include "BehaviorTree/TreeBuilder.hpp"
#include "BehaviorTree/BehaviorTree.hpp"
#include "BehaviorTree/BehaviorTreeVisualizer.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace bt {
namespace demo {

// ****************************************************************************
//! \brief Check if battery level is above threshold
// ****************************************************************************
class CheckBattery : public Action
{
public:
    explicit CheckBattery(Blackboard::Ptr blackboard)
        : Action(blackboard)
    {}

    Status onRunning() override
    {
        // Decrease the battery level at each check
        int current_battery = m_blackboard->getOr<int>("battery_level", 100);
        current_battery = std::max(0, current_battery - 5); // Consumption of 5%
        m_blackboard->set<int>("battery_level", current_battery);
        
        std::cout << "🔋 Checking battery: " << current_battery << "%\n";
        return (current_battery > 20) ? Status::FAILURE : Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief Patrol action that simulates robot movement
// ****************************************************************************
class Patrol : public Action
{
public:
    explicit Patrol(Blackboard::Ptr blackboard)
        : Action(blackboard)
    {}

    Status onRunning() override
    {
        // Consume battery during patrol
        int current_battery = m_blackboard->getOr<int>("battery_level", 100);
        current_battery = std::max(0, current_battery - 10); // Consumption of 10%
        m_blackboard->set<int>("battery_level", current_battery);
        
        std::cout << "🤖 Patrolling area... Battery: " << current_battery << "%\n";
        return Status::RUNNING;  // The patrol continues until interruption
    }
};

// ****************************************************************************
//! \brief Check for security threats
// ****************************************************************************
class DetectThreat : public Action
{
public:
    explicit DetectThreat(Blackboard::Ptr blackboard)
        : Action(blackboard)
    {}

    Status onRunning() override
    {
        bool threat = m_blackboard->getOr<bool>("threat_detected", false);
        std::cout << "🕵️ Scanning for threats...\n";
        return threat ? Status::SUCCESS : Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief Send alert when threat is detected
// ****************************************************************************
class SendAlert : public Action
{
public:
    Status onRunning() override
    {
        std::cout << "🚨 ALERT: Security threat detected! 🚨\n";
        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief Recharge robot's battery
// ****************************************************************************
class Recharge : public Action
{
public:
    explicit Recharge(Blackboard::Ptr blackboard)
        : Action(blackboard)
    {}

    Status onRunning() override
    {
        int current_battery = m_blackboard->getOr<int>("battery_level", 0);
        current_battery = std::min(100, current_battery + 20); // Recharge de 20%
        m_blackboard->set<int>("battery_level", current_battery);
        
        std::cout << "⚡ Recharging battery: " << current_battery << "% 🔌\n";
        return (current_battery >= 100) ? Status::SUCCESS : Status::RUNNING;
    }
};

// ****************************************************************************
//! \brief Factory to register custom actions.
// ****************************************************************************
class SecurityRobotFactory : public bt::NodeFactory
{
public:
    SecurityRobotFactory(bt::Blackboard::Ptr blackboard)
    {
        // Register actions with simplified syntax
        // Equivalent to registerNode<CheckBattery>("check_battery", blackboard);
        registerAction("check_battery", [blackboard]() {
            int current_battery = blackboard->getOr<int>("battery_level", 100);
            current_battery = std::max(0, current_battery - 5);
            blackboard->set<int>("battery_level", current_battery);
            
            std::cout << "🔋 Checking battery: " << current_battery << "%\n";
            return (current_battery > 20) ? Status::FAILURE : Status::SUCCESS;
        }, blackboard);

        // Register actions with simplified syntax
        // Equivalent to registerNode<Patrol>("patrol", blackboard);
        registerAction("patrol", [blackboard]() {
            int current_battery = blackboard->getOr<int>("battery_level", 100);
            current_battery = std::max(0, current_battery - 10);
            blackboard->set<int>("battery_level", current_battery);
            
            std::cout << "🤖 Patrolling area... Battery: " << current_battery << "%\n";
            return Status::RUNNING;
        }, blackboard);

        // Simplified node registration using templates
        registerNode<Recharge>("recharge", blackboard);
        registerNode<DetectThreat>("detect_threat", blackboard);
        registerNode<SendAlert>("send_alert");
    }
};

// ****************************************************************************
//! \brief Run the security robot demo.
// ****************************************************************************
void runDemo()
{
    auto blackboard = std::make_shared<bt::Blackboard>();
    blackboard->set<int>("battery_level", 100);
    blackboard->set<bool>("threat_detected", false);

    // Create the factory with custom actions
    auto factory = std::make_shared<SecurityRobotFactory>(blackboard);
    bt::TreeBuilder builder(factory);

    // Load the tree from the YAML file
    auto tree = builder.fromYAML("demos/security_robot/security_robot.yaml");
    if (!tree)
    {
        std::cerr << "Failed to load behavior tree from YAML\n";
        return;
    }

    // Initialize the connection with the visualizer
    BehaviorTreeVisualizer visualizer(*tree, "127.0.0.1", 9090);
    int connection_attempts = 0;
    while (!visualizer.isConnected() && connection_attempts < 3)
    {
        std::cout << "Waiting to connect to the visualizer application on port 9090...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        connection_attempts++;
    }

    // Simulation
    std::cout << "Starting security robot demo...\n";
    for (int i = 0; i < 20; ++i)
    {
        std::cout << "\n--- Tick " << i << " ---\n";

        // Simulate state changes
        if (i == 3) blackboard->set<bool>("threat_detected", true);
        if (i == 5) blackboard->set<int>("battery_level", 10);
        if (i == 10) blackboard->set<bool>("threat_detected", false);

        // Update the visualizer
        if (visualizer.isConnected())
        {
            visualizer.updateDebugInfo();
        }

        // Tick the tree
        tree->tick();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace demo
} // namespace bt

// ****************************************************************************
//! \brief Main function.
// ****************************************************************************
int main()
{
    try {
        bt::demo::runDemo();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}