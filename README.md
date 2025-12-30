# Génération de Labyrinthe - Parallélisation MPI

Projet de parallélisation d'un algorithme de génération de labyrinthe complexe utilisant MPI (Message Passing Interface) avec décomposition de domaine.

## Vue d'ensemble

Ce projet implémente un algorithme de génération de labyrinthe itératif qui construit progressivement les murs d'un labyrinthe en partant d'une grille vide avec des îlots initiaux. Le projet propose deux implémentations :

- **Version séquentielle** : Implémentation de référence pour comparaison de performances
- **Version parallèle MPI** : Implémentation parallèle utilisant une décomposition de domaine 1D avec échange de lignes fantômes (ghost rows)

L'algorithme génère des labyrinthes de taille configurable avec un nombre personnalisable d'îlots initiaux, puis calcule itérativement les positions des murs selon des règles de constructibilité.

## Caractéristiques techniques

- Décomposition de domaine 1D par bandes horizontales
- Communication MPI avec échange de lignes fantômes (halo exchange)
- Support de visualisation graphique via X11
- Mesures de performances automatisées (scalabilité forte et faible)
- Sauvegarde du labyrinthe généré au format binaire pour visualisation ultérieure

## Prérequis

- **Compilateur C** : GCC ou compatible avec support C99
- **MPI** : OpenMPI ou MPICH (bibliothèque MPI 3.0+)
- **Bibliothèque X11** : Pour la visualisation graphique (optionnelle)
  - Sur Debian/Ubuntu : `sudo apt-get install libx11-dev`
  - Sur Fedora/RHEL : `sudo yum install libX11-devel`

## Installation

### Compilation

Le projet utilise un Makefile standard. Pour compiler tous les exécutables :

```bash
make
```

Cette commande génère trois exécutables :

- `gen_lab` : Version séquentielle de génération de labyrinthe
- `gen_lab-parallel` : Version parallèle MPI
- `chemin_lab` : Outil de visualisation et résolution de labyrinthe

### Options de compilation

Les options de compilation sont définies dans le Makefile :
- `-O2` : Optimisation niveau 2
- `-g` : Informations de débogage
- `-Wall` : Avertissements compilateur
- `-std=c99` : Standard C99

Pour nettoyer les fichiers compilés :

```bash
make clean
```

## Utilisation

### Génération séquentielle

```bash
./gen_lab [nbilots] [N] [M]
```

**Paramètres :**
- `nbilots` : Nombre d'îlots initiaux (défaut : 20)
- `N` : Hauteur du labyrinthe en nombre de lignes (défaut : 400)
- `M` : Largeur du labyrinthe en nombre de colonnes (défaut : 600)

**Exemple :**
```bash
./gen_lab 25 500 800
```

### Génération parallèle MPI

```bash
mpirun -np <nombre_processus> ./gen_lab-parallel [nbilots] [N] [M]
```

**Paramètres :** Identiques à la version séquentielle

**Exemples :**

```bash
# Exécution avec 2 processus
mpirun -np 2 ./gen_lab-parallel 20 400 600

# Exécution avec 4 processus (option --oversubscribe si nombre de processus > nombre de cœurs)
mpirun -np 4 --oversubscribe ./gen_lab-parallel 20 400 600

# Exécution avec paramètres personnalisés
mpirun -np 3 --oversubscribe ./gen_lab-parallel 30 800 1200
```

**Note :** La version parallèle nécessite que la hauteur `N` soit divisible par le nombre de processus pour une distribution équilibrée.

### Visualisation du labyrinthe

Après génération, le labyrinthe est sauvegardé dans `laby.lab` au format binaire. Pour visualiser et résoudre le labyrinthe :

```bash
./chemin_lab
```

**Fonctionnalités de visualisation :**
- Affichage graphique du labyrinthe (murs en noir, chemins en blanc)
- Recherche aléatoire de chemin (option en ligne de commande)
- Recherche du chemin le plus court via algorithme d'inondation
- Navigation interactive (appuyez sur 'q' pour quitter)

**Exemple avec recherche aléatoire :**
```bash
./chemin_lab 1
```

## Architecture et algorithme

### Algorithme de génération

L'algorithme procède par itérations successives :

1. **Initialisation** : Création d'une grille vide avec murs périphériques
2. **Placement d'îlots** : Ajout aléatoire d'îlots initiaux (obstacles isolés)
3. **Identification des cases constructibles** : Une case est constructible si elle est vide et si elle a exactement un voisin mur parmi ses 8 voisins
4. **Construction itérative** :
   - Sélection aléatoire d'une case constructible parmi toutes les cases disponibles
   - Transformation de cette case en mur
   - Mise à jour des 8 voisins (ajout/retrait de la liste des constructibles)
   - Répétition jusqu'à ce qu'il n'y ait plus de cases constructibles

### Parallélisation MPI

#### Décomposition de domaine 1D

Le labyrinthe de taille `N × M` est divisé en bandes horizontales :
- Chaque processus `i` gère `N_loc = N / size` lignes consécutives
- Le processus gère les lignes globales `[i × N_loc, (i+1) × N_loc - 1]`

#### Lignes fantômes (Ghost Rows)

Chaque processus maintient deux lignes fantômes supplémentaires :
- Ligne `0` : Fantôme supérieur (copie de la dernière ligne du processus voisin du haut)
- Ligne `N_loc+1` : Fantôme inférieur (copie de la première ligne du processus voisin du bas)

Ces lignes permettent le calcul correct de la constructibilité des cases frontières sans accès direct aux données des processus voisins.

#### Communication MPI

L'échange de lignes fantômes utilise `MPI_Sendrecv` pour garantir l'absence de deadlock :

```c
void update_ghosts(int rows, int cols, int l[rows][cols], int rank, int size) {
    // Échange vers le haut et vers le bas
    MPI_Sendrecv(...);
}
```

#### Synchronisation globale

L'algorithme nécessite une synchronisation globale à chaque itération :
- Calcul du nombre total de cases constructibles via `MPI_Allreduce`
- Sélection aléatoire locale d'une case parmi les constructibles locales
- Communication des frontières après chaque modification locale

### Complexité

- **Séquentiel** : O(N × M × k) où k est le nombre moyen d'itérations (dépend du nombre d'îlots)
- **Parallèle** : O((N/p) × M × k + communication) où p est le nombre de processus

## Mesures de performances

Un script automatisé permet d'effectuer des mesures de scalabilité :

```bash
./mesures_performances.sh
```

### Types de mesures

**Scalabilité forte :**
- Taille de problème fixe (400×600 par défaut)
- Variation du nombre de processus (1, 2, 3, 4)
- Mesure du temps d'exécution pour évaluer l'accélération

**Scalabilité faible :**
- Charge de travail proportionnelle au nombre de processus
- 1 processus : 400×600 (base)
- 2 processus : 565×848 (surface ×2)
- 3 processus : 692×1038 (surface ×3)
- 4 processus : 800×1200 (surface ×4)
- Mesure du temps d'exécution pour évaluer l'efficacité parallèle

### Résultats

Les résultats sont sauvegardés dans `mesures_results.txt` avec le format suivant :
```
=== SCALABILITÉ FORTE ===
Processus | Exécution | Temps (s)
...

=== SCALABILITÉ FAIBLE ===
Processus | Dimensions | Exécution | Temps (s)
...
```

## Structure du projet

```
.
├── gen_lab.c                  # Implémentation séquentielle
├── gen_lab-parallel.c         # Implémentation parallèle MPI
├── chemin_lab.c               # Visualisation et résolution
├── graph.c                    # Bibliothèque graphique X11
├── graph.h                    # En-tête bibliothèque graphique
├── Makefile                   # Fichier de compilation
├── mesures_performances.sh    # Script de mesures automatisées
├── mesures_results.txt        # Résultats des mesures (généré)
├── laby.lab                   # Labyrinthe généré (binaire, généré)
├── rapport_projet_lab.tex     # Rapport LaTeX du projet
├── rapport_projet_lab.pdf     # Rapport compilé
└── README.md                  # Ce fichier
```

## Format de fichier

Le labyrinthe généré est sauvegardé dans `laby.lab` au format binaire :
- `sizeof(int)` : Hauteur N
- `sizeof(int)` : Largeur M
- `N × M × sizeof(int)` : Matrice du labyrinthe
  - `0` : Mur
  - `1` : Case vide (chemin possible)

## Limitations connues

- La version parallèle nécessite que `N` soit divisible par le nombre de processus
- La visualisation X11 nécessite un serveur X actif (non disponible sur systèmes headless)
- L'algorithme génère des labyrinthes avec une seule solution (connexe)

## Développement

### Prochaines améliorations possibles

- Support de dimensions non divisibles par le nombre de processus
- Décomposition 2D pour meilleure localité
- Optimisation de la communication (réduction du nombre d'échanges)
- Support d'autres algorithmes de génération (Kruskal, Prim, etc.)

## Auteur

Junior Koudogbo

## Licence

Ce projet est fourni à des fins éducatives.
