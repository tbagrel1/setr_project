# setr_project

## Important

Il faut désactiver la capture d'exceptions par Visual Studio si on lance le programme depuis l'IDE. En effet, VS va capturer les exceptions levées **quand bien même un `catch (...)` serait présent pour les traiter**. Par défaut, si on lance le programme depuis VS, ce dernier s'arrête si la connexion au serveur est impossible par exemple (alors que le code pour gérer ce cas est présent).

Pour désactiver la capture des exceptions par l'IDE, il faut lancer le programme, et dans la barre de débuggage, dans l'onglet *Paramètres d'exception*, décocher *C++ Exceptions*

## Builds testés

+ Debug x86 Mobile Emulator 10.0.15254.0 720p 5 inch 1GB (Natif uniquement)
+ Debug x86 Ordinateur local (Natif uniquement)

Parfois la carte ne semble pas s'initialiser sur PC, il suffit de relancer l'application.
