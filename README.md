# Behavior Tree

## Description

A C++ library to implement Behavior Trees with:

- Support for importing and exporting YAML (our library format) and XML formats ([BehaviorTree.CPP](https://github.com/BehaviorTree/BehaviorTree.CPP/wiki/02.-BT-Editor-&-YAML-File-Format) format)
- Basic nodes:
  - Composites: Sequence, Selector, Parallel
  - Decorators: Inverter, Succeeder, Repeater, RepeatUntilFail
  - Leaves: Custom Actions and Conditions
- Export trees to DOT format for graphical visualization
- Fluent API for building trees in C++

## Dependencies

- yaml-cpp
- tinyxml2

```bash
sudo apt-get install libyaml-cpp-dev libtinyxml2-dev
```

## Build

```bash
make
```

## Usage

### Building a tree in C++

```cpp
#include <BehaviorTree/BehaviorTree.hpp>
#include <BehaviorTree/TreeBuilder.hpp>

// Define your custom action
class MoveToAction : public BehaviorTree::Node {
    Status update() override {
        // Move robot to target position
        return Status::Success;
    }
};

// Register your custom nodes
BehaviorTree::TreeBuilder builder;
builder.registerNodeType<MoveToAction>("MoveTo");

// Build the tree
auto tree = builder.createTreeFromText(R"(
    type: sequence
    name: patrol_sequence
    children:
    - type: MoveTo
      name: move_to_waypoint
    - type: condition
      name: check_area
)");

// Run the tree
while (true) {
    tree->update();
}
```

## Examples

Check the `demos/security_robot/` folder for a complete example of a security robot using a behavior tree.

## Tests

Unit tests are available in the `tests/` folder and demonstrate the usage of various features.

## License

MIT License