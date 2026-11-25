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

## Étape 5 — Projectile Cyclone

J’ai remplacé la sphère provisoire par un vrai projectile avec son `RigidBody`
Cyclone et sa `CollisionSphere`. Le profil standard pèse 1 kg avec un rayon de
0,28 m, et le lourd 2 kg avec un rayon de 0,36 m. Chaque projectile a aussi son
inertie, son amortissement et la gravité. La barre d’espace lance le projectile
choisi avec la vitesse réglée au lanceur ; c’est une pression, donc la garder
enfoncée ne crée pas une suite de tirs.

J’ai ajouté une boucle physique Cyclone temporaire à pas fixe de 1/60 s. Elle
intègre le corps, détecte le contact avec le sol puis le résout avec Cyclone.
La sphère dessinée suit directement la position du corps. Les tests vérifient
les deux profils, la vitesse initiale et le contact avec le plan. Les 30 tests
passent en Debug et en Release.

## Étape 6 — Aperçu de trajectoire Cyclone

J’ai ajouté un aperçu vert qui suit la visée actuelle du lanceur. Il est calculé
avec un corps Cyclone temporaire, au même pas fixe que le tir, et s’arrête au
premier contact détecté avec le sol ou une boîte. La prédiction garde une copie
des formes statiques fournies; elle ne fait pas avancer le projectile réel et
ne résout pas de rebond. Pour l’instant, comme la scène n’a pas encore ses
obstacles de niveau, l’aperçu montre le contact avec le sol. Les quatre tests
couvrent le contact au sol, le contact avec une boîte, la limite de calcul et
la différence de rayon entre les projectiles. Les 34 tests passent en Debug
et en Release.

## Étape 7 — Pilier et cube

J’ai construit la première cible avec deux blocs Cyclone indépendants : un
pilier de 3 kg et un cube de 1,5 kg posés l’un sur l’autre. Ils ont leur
collider boîte, leur inertie de cuboïde et les réglages de gravité,
d’amortissement, de friction et de rebond prévus. La scène les affiche avec
leur orientation physique. Au démarrage, ils restent au repos; un impact les
réveille et le projectile interagit réellement avec les blocs et le sol. La
prédiction utilise aussi une copie de leur pose et de leur forme.

J’ai testé les dimensions et masses, leur repos sur 780 pas, puis un tir
excentré après cette attente : le pilier tourne et le cube se sépare. Les 37
tests passent en Debug et en Release.

## Étape 8 — Monde physique Cyclone

J’ai ajouté `PhysicsWorld` pour regrouper le pas de simulation au lieu de
laisser le niveau intégrer et résoudre les collisions lui-même. Le monde
ne possède pas les objets : il emprunte leurs corps et leurs colliders typés,
et les blocs restent détenus par `SceneObjects`. Il peut enregistrer des
sphères, des boîtes et des plans séparément, avancer seulement les corps
mobiles, puis détecter et résoudre ensemble les contacts avec Cyclone.

J’ai aussi remplacé l’ancien pas physique du niveau par cet orchestrateur. Le
projectile est enregistré seulement pendant son pas, puis désinscrit avant de
pouvoir être détruit. Si le tampon de contacts ne permet pas de vérifier toutes
les paires, le pas le signale et le jeu affiche un avertissement. Les nouveaux
tests couvrent les formes, le sol, les corps fixes, les contacts, la capacité
du tampon et la désinscription. Les tests de repos et d’impact du niveau
continuent aussi de passer. Les 47 tests passent en Debug et en Release.

## Étape 9 — Événements de collision

J’ai ajouté les événements `Enter` et `Exit` autour des contacts calculés par
Cyclone. Le résolveur transmet seulement les vitesses préparées à un observateur
léger; `PhysicsWorld` en copie les identifiants, le point de contact, la normale
et la vitesse avant que Cyclone ne modifie les corps. Les événements sont ensuite
envoyés après la résolution, sans conserver de pointeur vers les contacts.

Une paire n’envoie un `Enter` qu’à partir de 1 m/s. Elle envoie un `Exit` après
trois pas de contact calme ou deux pas complets sans contact; une détection
incomplète ne compte pas comme une séparation. Les paires sont ordonnées avec
leur génération d’identifiant, et le sol utilise un identifiant spécial sans
corps. Les écouteurs peuvent se désabonner pendant l’envoi. J’ai aussi ajouté
une file de suppressions : le niveau désinscrit les objets du monde avant de
les détruire, après la résolution et l’envoi des événements. Le test du pilier
et du cube confirme qu’ils ne produisent pas d’événements au repos prolongé.
Les 63 tests passent en Debug et en Release.