# Avancement du projet

## Étape 1 — Mise en place du projet

Pour commencer, j’ai regardé la solution déjà présente et je l’ai organisée
pour avoir les trois parties de base :

- **Cyclone** contient le moteur physique fourni.
- **SiegeLabCore** contiendra le code principal du jeu.
- **SiegeLabGame** est le programme qu’on lance.
- **SiegeLabTests** servira à lancer les tests.

Pour vérifier que tout fonctionnait, j’ai mis une première fenêtre raylib avec
le nom du jeu, et un petit test qui vérifie que `2 + 3` donne `5`. J’ai
compilé le projet en Debug et en Release, puis lancé le test dans les deux
versions. Tout a réussi.