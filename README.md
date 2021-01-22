# setr_project

## Auteurs

+ Thomas BAGREL <thomas.bagrel@telecomnancy.eu>
+ Timothée ADAM <timothee.adam@telecomnancy.eu>

## /!\ Important

Il faut désactiver la capture d'exceptions par Visual Studio si on lance le programme depuis l'IDE. En effet, VS va capturer les exceptions levées **quand bien même un `catch (...)` serait présent pour les traiter**. Par défaut, si on lance le programme depuis VS, ce dernier s'arrête si la connexion au serveur est impossible par exemple (alors que le code pour gérer ce cas est présent).

Pour désactiver la capture des exceptions par l'IDE, il faut lancer le programme, et dans la barre de débuggage, dans l'onglet *Paramètres d'exception*, décocher *C++ Exceptions*

## Builds testés

+ Release x86 Mobile Emulator 10.0.15254.0 720p 5 inch 1GB (Natif uniquement)
+ Debug x86 Mobile Emulator 10.0.15254.0 720p 5 inch 1GB (Natif uniquement)
+ Debug x86 Ordinateur local (Natif uniquement)

Parfois la carte ne semble pas s'initialiser sur PC, il suffit de relancer l'application.

## Fonctionnalités

+ Localisation en utilisant le GPS ou manuellement, en entrant des coordonnées. Un message d'information apparaît si la localisation est désactivée sur l'appareil, avec un lien vers les settings
+ Récupération des informations sur les motes depuis l'IOTLab, et comparaison avec la liste statique des motes fournies pour détecter lesquelles sont en fonctionnement et lesquelles sont hors ligne. En cas d'erreur lors de la communication avec l'API, un message s'affiche en bas de l'application
+ Affichage du nom de la mote active la plus proche de l'utilisateur, ainsi que de sa température, le lieu où elle est installée, et sa latitude / longitude
+ Affichage sur une carte dynamique des différentes motes ainsi que de l'utilisateur
    + la mote active la plus proche de l'utilisateur est affichée par un point vert
    + les autres motes actives sont affichées par des points rouges
    + les motes hors-ligne sont affichées par des points gris
    + l'utilisateur est affiché sous forme d'un bonhomme bleu

## Fonctionnement interne

L'application utilise un thread pour l'UI, ainsi que deux threads d'arrière plan : `readSensorsThread` et `computeThread`. La partie GPS est quant à elle gérée de façon asynchrone et événementielle à partir de l'UI thread grâce au framework .NET.

La synchronisation entre les threads se fait via les objets du type générique `Mutexed<T>` que nous avons implémenté, qui permet de manipuler facilement des variables de façon thread-safe.

Le GPS ainsi que le thread `readSensorsThread` mettent à jour les variables thread-safe `readSensorsResult` et `userPositionResult` avec une fréquence assez lente. `computeThread` lit avec une fréquence élevée d'éventuelles modifications sur ces deux structures résultat, et s'il en détecte, lance les différents calculs et met à jour la variable thread-safe `computeResult`. Enfin, l'UI thread lit de façon périodique les éventuelles modifications sur `computeResult` et met à jour l'UI en conséquence. La seule autre variable thread-safe manipulée par l'UI thread est `useGps`, qui permet d'indiquer si le GPS doit mettre-à-jour ou non la variable `userPositionResult` quand un changement de position est détecté.

## Difficultés rencontrées

Le projet a été très laborieux à utiliser car les consignes spécifiques (à savoir utiliser des background threads gérés à la main, gérer manuellement la synchronisation des threads, ...) semblent souvent aller à l'encontre des patterns de l'API .NET et de la programmation moderne en UWP. Par conséquent, il est très complexe de trouver des exemples et de la documentation C++/CX sur internet (puisque Microsoft promouvoit aujourd'hui C#, interdit dans le cadre de ce projet, et C++/WinRT, incompatible avec la version du SDK demandée), et de nombreux passages dans le code sont extrêmement verbeux pour réaliser les conversions incessantes entre C++ et les types de l'API .NET.

L'utilisation massive d'asynchrone dans l'API .NET supprime normalement le besoin de background threads gérés à la main (puisque le framework gère lui-même un pool de threads dédiés à la réalisation de tâches de fond). Nous avons donc du forcer ici l'exécution des tâches asynchrones de manière synchrone sur les background threads gérés à la main, afin de respecter les consignes du projet et ce que nous avions vu en TP.

