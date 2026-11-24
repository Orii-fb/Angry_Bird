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

## Étape 2 — Pool d’objets générique

J’ai ajouté un `ObjectPool` à capacité fixe, sans dépendance au moteur
physique ni à raylib. Chaque objet reçoit un identifiant avec un numéro de
génération : si un emplacement est libéré puis réutilisé, l’ancien identifiant
ne permet pas d’accéder au nouvel objet. Le pool permet aussi de parcourir les
objets actifs et de les supprimer ou de tout vider.

J’ai aussi ajouté `GameEntity` et `SceneObjects` pour stocker des entités
polymorphes dans ce pool. La création est limitée aux classes concrètes qui
hérent de `GameEntity`. J’ai d’abord lancé les tests et vérifié qu’ils
échouaient, puis j’ai ajouté le code nécessaire pour les faire réussir. Les
tests couvrent la création, la capacité, la réutilisation, la stabilité des
adresses, les anciens identifiants et la destruction des objets. Les neuf
tests passent en Debug et en Release.

## Étape 3 — Caméra orbitale

J’ai ajouté une caméra qui tourne autour d’un point fixe sans influencer
l’orientation du lanceur. Le clic droit avec déplacement de la souris fait
tourner la vue, la molette règle le zoom, et les flèches ainsi que le pavé
numérique permettent de la déplacer aussi au clavier. L’inclinaison et la
distance sont limitées pour garder une vue utilisable. `CameraManager` peut
changer de contrôleur par son interface, sans dépendre directement de la
caméra orbitale. Les dix nouveaux tests passent en Debug et en Release.