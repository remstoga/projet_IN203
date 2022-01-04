# Projet

## Question 2.1

Mesure du temps en séquentiel

| Processus mesuré (itération) | Temps (en ms) |
| ---------------------------- | ------------- |
| Avec affichage               | 60            |
| Sans affichage               | 32            |
| Affichage seul               | 24.5          |

On remarque que sans affichage, les itérations s'effectuent quasiment deux fois plus rapidement. L'affichage seul est en effet très gourmand en temps (24 millisecondes à peu près pour mettre à jour le cadre)

## Question 2.2

Mesure du temps en Parallélisation affichage contre simulation

| Processus mesuré (itération) | Temps (en ms) |
| ---------------------------- | ------------- |
| Affichage                    | 27            |
| temps d'une boucle           | 35            |

Le temps d'affichage pur est sensiblement le même, ce qui est normal, par contre le temps d'une boucle est quasiment divisé par deux : en effet, lorsque le processus de rang 0 est en train d'afficher, celui dédié au calcul peut avancer dans la propagation de l'épidémie en parallèle. Le temps en plus provient des échanges de messages et des copies d'information : les statistiques de la grille sont copiées dans un tableau d'entier basique, afin que l'envoi avec mpi soit simple, puis sont récupérées et recopiées dans une instance de grille.

## Question 2.3

| Temps d'une boucle | 20 ms |
| ------------------ | ----- |

Le programme effectue en moyenne 2 boucles entre chaque affichage, et celles-ci de 20 millisecondes en moyenne, donc on a effectivement une amélioration par rapport à l'affichage synchrone. Le retrait du coût de copie et d'envoi permet une meilleure rapidité.

## Question 2.4

| Nombre de thread (individus constants)       | Temps d'une boucle (en ms) |
| -------------------------------------------- | -------------------------- |
| 2                                            | 15                         |
| 3                                            | 15                         |
| Jusqu'à 12 (nombre de threads de ma machine) | 15                         |

Lorsque l'on passde de 1 thread à 2 threads on constate bien une nette amélioration, toutefois rajouter des threads n'améliore pas le temps d'une boucle. Le temps de la boucle for doit devenir négligeable par rapport au reste, il devient nécessaire de paralléliser sur une plus grande échelle

| Nombre de thread (20 000 individus par thread) | Temps d'une boucle (en ms) |
| ---------------------------------------------- | -------------------------- |
| 2                                              | 5                          |
| 3                                              | 10                         |
| 4                                              | 11                         |
| 5                                              | 14                         |
| 6                                              | 17                         |
| 8                                              | 20                         |
| 10                                             | 25                         |
| 12                                             | 30                         |

## Question 2.5

Ici processus sous-entend processus s'occupant de la simulation, et non la simulation

| Nombre de processus (individus constants) | Temps d'une boucle (en ms) |
| ----------------------------------------- | -------------------------- |
| 1                                         | 20                         |
| 2                                         | 12                         |
| 4                                         | 9                          |
| 8      (oversubscribe donc sous-optimal)  | 12                         |

| Nombre de processus (20 000 individus par processus) | Temps d'une boucle (en ms) |
| ---------------------------------------------------- | -------------------------- |
| 1                                                    | 3.5                        |
| 2                                                    | 4.4                        |
| 4                                                    | 7                          |
| 8 (oversubscribe également)                          | 23                         |

On arrive à faire descendre le temps d'une boucle assez bas avec 4 processus. Au delà de 8, il est nécessaire d'utiliser l'option --oversubscribe, qui n'utilise pas réellement 8 processus mais qui le simule, donc qui devient moins optimal.

## Question 2.5.1

On fera en sorte que le nombre de thread global n'excède pas 12, limite de ma machine. Je n'ai pas eu l'occasion d'éxécuter le code sur plusieurs machines en réseau, donc je me contenterai d'une seule.

A population constante :

| Nombre de processus | Nombre de threads | Temps d'une boucle (en ms) |
| ------------------- | ----------------- | -------------------------- |
| 2                   | 2                 | 8                          |
| 2                   | 3                 | 6                          |
| 2                   | 4                 | 4                          |
| 2                   | 5                 | 4                          |
| 4                   | 2                 | 8                          |

Les temps de boucle deviennent extremement courts, avec de très bons temps pour 2 processus et 4 ou 5 threads.

A population variable (20'000 x processus x threads)

| Nombre de processus | Nombre de threads | Temps d'une boucle (en ms) |
| ------------------- | ----------------- | -------------------------- |
| 2                   | 2                 | 5                          |
| 2                   | 3                 | 7                          |
| 2                   | 4                 | 9                          |
| 2                   | 5                 | 11                         |
| 4                   | 2                 | 13                         |

Le temps semble augmenter linéairement, ce qui est cohérent.

## Bilan

Nous sommes passés de 60 millisecondes pour un tour de boucle à 4 millisecondes, ce qui est extrèmement rapide. En effet après avoir detecté l'affichage comme processus long et compliqué à paralléliser, nous avons pu l'isoler pour se concentrer sur la partie algorithmique. Utiliser les processus OU les threads apportent des améliorations, mais c'est vraiment la combinaison des deux qui accélère de façon considérable le temps d'une boucle. Ma machine étant limité à 12 threads et en devant prendre en compte l'affichage, je n'ai pas pu aller très loin dans les nombres, mais avec plusieurs machines j'aurais pu observer à un moment les coûts de communication.

Ce projet m'a permis de consolider mes acquis sur openMP et MPI, et j'ai appris à quoi celà servait réellement d'utiliser les deux combinés, en voyant l'accéleration sur une simulation conséquente comme celle-ci.