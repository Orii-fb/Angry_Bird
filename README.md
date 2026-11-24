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

## Étape 4 — Réglage du lanceur

J’ai ajouté le lanceur avec un point de départ fixe et une direction calculée
à partir des angles d’Euler. Les touches A et D règlent l’azimut, W et S
l’élévation, Q et E la vitesse, et 1 ou 2 choisit le projectile. Les réglages
restent dans les limites prévues et la direction renvoyée est normalisée.
J’ai choisi une vitesse de rotation de 60 degrés par seconde et un changement
de vitesse de 5 m/s par seconde quand une touche est maintenue. J’ai aussi
affiché le lanceur et un repère de visée dans la scène ; le déplacement de la
caméra ne change pas cette direction. Les sept tests du lanceur ont d’abord
échoué avec le squelette, puis passent après l’implémentation. Les 26 tests
passent maintenant en Debug et en Release.