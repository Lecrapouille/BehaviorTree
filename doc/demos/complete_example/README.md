# Complete Example

Cet exemple démontre l'utilisation complète de la bibliothèque BehaviorTree avec un personnage de jeu simple.

## Description

L'exemple simule un personnage de jeu qui doit gérer plusieurs états :
- Faim
- Fatigue
- Santé
- Énergie
- Présence d'ennemis

## Fonctionnalités

- Gestion des états du personnage
- Actions basiques (manger, dormir, combattre, se soigner)
- Conditions pour vérifier l'état du personnage
- Mise à jour aléatoire de l'état
- Visualisation de l'arbre de comportement

## Structure du code

- `CharacterState` : Structure contenant l'état du personnage
- `Character` : Classe principale gérant les actions et conditions
- `main()` : Point d'entrée qui crée et exécute l'arbre de comportement

## Compilation

```bash
mkdir build && cd build
cmake ..
make
```

## Exécution

```bash
./complete_example
```

## Visualisation

L'arbre de comportement peut être visualisé en utilisant le visualiseur intégré qui se connecte sur le port 9090.