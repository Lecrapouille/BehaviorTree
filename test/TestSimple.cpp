#include "BehaviorTree/BehaviorTree.hpp"
#include <iostream>

class Hello : public bt::Node
{
public:

    Hello(std::string const& message)
        : m_message(message)
    {}

    virtual /*bt::Status*/ void onStart() override
    {
        std::cout << "Hello node onStart: "
                  << m_message << std::endl;
        //return bt::Status::RUNNING;
    }

    virtual bt::Status onRunning() override
    {
        std::cout << "Hello node onRunning: "
                  << m_message << std::endl;
        return bt::Status::SUCCESS;
    }

    virtual void onHalted(bt::Status status) override
    {
        std::cout << "Hello node onHalted with status "
                  << bt::to_string(status) << ": "
                  << m_message << std::endl;
    }

private:

    std::string m_message;
};

// g++ -Wall -Wextra -Wshadow --std=c++14 -I../include TestSimple.cpp
int main()
{
    bt::BehaviorTree tree;
    bt::Sequence& sequence = tree.setRoot<bt::Sequence>();
    /*Hello& sayHello =*/ sequence.addChild<Hello>("Hello");
    /*Hello& sayHelloAgain =*/ sequence.addChild<Hello>("World!");

    do {
        std::cout << "========================" << std::endl;
        bt::Status status = tree.tick();
        std::cout << bt::to_string(status) << std::endl;
    } while (!tree.isTerminated());

    return 0;
}
