#include "BehaviorTreeViewer.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    BehaviorTreeViewer viewer;
    
    if (!viewer.initialize()) {
        std::cerr << "Échec de l'initialisation du visualiseur" << std::endl;
        return 1;
    }

    viewer.run();
    return 0;
} 