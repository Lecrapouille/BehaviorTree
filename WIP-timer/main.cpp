#include "TimerQueue.hpp"
#include <iostream>

int main()
{
    TimerQueue tq;

    // Ajouter une tâche
    auto id1 = tq.add(std::chrono::seconds(5), [](bool canceled) {
        if (canceled)
        {
            std::cout<< "id1 canceld" << std::endl;
        }
        else
        {
            std::cout << "Tâche exécutée! 5s" << std::endl;
        }
    });

    // Ajouter une tâche
    auto id2 = tq.add(std::chrono::seconds(1), [](bool canceled) {
        if (canceled)
        {
            std::cout<< "id2 canceld" << std::endl;
        }
        else
        {
            std::cout << "Tâche exécutée! 1s" << std::endl;
        }
    });

    // Ajouter une tâche
    auto id3 = tq.add(std::chrono::seconds(3), [](bool canceled) {
        if (canceled)
        {
            std::cout<< "id3 canceld" << std::endl;
        }
        else
        {
            std::cout << "Tâche exécutée! 3s" << std::endl;
        }
    });

    // Annuler une tâche
    tq.cancel(id2);

    // Ajouter une tâche périodique
    std::function<void(bool)> periodic;
    periodic = [&](bool canceled) {
        if(canceled) return;
        std::cout << "Tâche périodique" << std::endl;
        tq.add(std::chrono::seconds(1), periodic);
    };
    tq.add(std::chrono::seconds(1), periodic);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    return 0;
}
