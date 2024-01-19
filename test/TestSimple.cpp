#include "BehaviorTree/BehaviorTree.hpp"
#include <iostream>

class Action : public bt::Node
{
public:
    Status update() override
    {
        std::cout << "Hello, World!" << std::endl;
        return bt::Node::Status::Success;
    }
};

// g++ -Wall -Wextra -Wshadow --std=c++14 -I../include TestSimple.cpp
int main()
{
    bt::BehaviorTree tree;
    auto sequence = std::make_shared<bt::Sequence>();
    auto sayHello = std::make_shared<Action>();
    auto sayHelloAgain = std::make_shared<Action>();
    sequence->addChild(sayHello);
    sequence->addChild(sayHelloAgain);
    tree.setRoot(sequence);
    tree.update();

    return 0;
}