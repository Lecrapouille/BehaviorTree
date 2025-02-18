//*****************************************************************************
// Security Robot Demo - Shows various behavior tree nodes in action
//*****************************************************************************

#include "BehaviorTree/TreeBuilder.hpp"
#include "BehaviorTree/BehaviorTree.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace bt {
namespace demo {

// ****************************************************************************
//! \brief Check if battery level is above threshold
// ****************************************************************************
class CheckBattery : public Leaf
{
public:
    explicit CheckBattery(Blackboard::Ptr blackboard)
        : Leaf(blackboard)
    {}

    Status onRunning() override
    {
        int battery = m_blackboard->getOr<int>("battery_level", 0);
        std::cout << "Checking battery: " << battery << "%\n";
        return (battery > 20) ? Status::SUCCESS : Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief Patrol action that simulates robot movement
// ****************************************************************************
class Patrol : public Leaf
{
public:
    Status onRunning() override
    {
        std::cout << "Patrolling area...\n";
        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief Check for security threats
// ****************************************************************************
class DetectThreat : public Leaf
{
public:
    explicit DetectThreat(Blackboard::Ptr blackboard)
        : Leaf(blackboard)
    {}

    Status onRunning() override
    {
        bool threat = m_blackboard->getOr<bool>("threat_detected", false);
        std::cout << "Scanning for threats...\n";
        return threat ? Status::SUCCESS : Status::FAILURE;
    }
};

// ****************************************************************************
//! \brief Send alert when threat is detected
// ****************************************************************************
class SendAlert : public Leaf
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
class Recharge : public Leaf
{
public:
    explicit Recharge(Blackboard::Ptr blackboard)
        : Leaf(blackboard)
    {}

    Status onRunning() override
    {
        std::cout << "Recharging battery...\n";
        m_blackboard->set<int>("battery_level", 100);
        return Status::SUCCESS;
    }
};

// ****************************************************************************
//! \brief Factory to register custom actions.
// ****************************************************************************
class SecurityRobotFactory : public NodeFactory
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] blackboard Shared blackboard for data
    // ------------------------------------------------------------------------
    explicit SecurityRobotFactory(std::shared_ptr<Blackboard> blackboard)
        : m_blackboard(blackboard)
    {
        registerNode("check_battery", [this]() { return std::make_unique<CheckBattery>(m_blackboard); });
        registerNode("patrol", []() { return std::make_unique<Patrol>(); });
        registerNode("detect_threat", [this]() { return std::make_unique<DetectThreat>(m_blackboard); });
        registerNode("send_alert", []() { return std::make_unique<SendAlert>(); });
        registerNode("recharge", [this]() { return std::make_unique<Recharge>(m_blackboard); });
    }

private:
    std::shared_ptr<Blackboard> m_blackboard;
};

// ****************************************************************************
//! \brief Run the security robot demo.
// ****************************************************************************
void runDemo()
{
    auto blackboard = std::make_shared<Blackboard>();
    blackboard->set<int>("battery_level", 100);
    blackboard->set<bool>("threat_detected", false);

    // Create the factory with custom actions
    SecurityRobotFactory factory(blackboard);
    TreeBuilder::setFactory(&factory);

    // Load the tree from the YAML file
    auto tree = TreeBuilder::fromYAML("demo/security_robot/security_robot.yaml");
    if (!tree) {
        std::cerr << "Failed to load behavior tree from YAML\n";
        return;
    }

    // Simulation
    std::cout << "Starting security robot demo...\n";
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "\n--- Tick " << i << " ---\n";

        // Simulate state changes
        if (i == 3) blackboard->set<bool>("threat_detected", true);
        if (i == 5) blackboard->set<int>("battery_level", 10);

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
    bt::demo::runDemo();
    return 0;
}