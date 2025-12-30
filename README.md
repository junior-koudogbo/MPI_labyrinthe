# Parallélisation de la génération de labyrinthe avec MPI

Ce projet présente l'implémentation parallèle d'un algorithme de génération de labyrinthe complexe en utilisant MPI (Message Passing Interface) pour la programmation parallèle en mémoire distribuée.

## Description

La génération de labyrinthes complexes nécessite des calculs intensifs pour construire itérativement les murs selon des règles de constructibilité. Ce projet implémente une version séquentielle de référence et une version parallélisée avec MPI utilisant une décomposition de domaine 1D, permettant d'exploiter efficacement les architectures distribuées et les clusters HPC.

### Caractéristiques

- **Version séquentielle** (`gen_lab.c`) : Implémentation de référence
- **Version parallèle** (`gen_lab-parallel.c`) : Version parallélisée avec MPI utilisant décomposition de domaine 1D et échange de lignes fantômes (ghost rows)
- Génération de labyrinthes au format binaire (`.lab`)
- Outil de visualisation graphique avec résolution de chemin
- Script de mesure de performances (scalabilité forte et faible)

## Prérequis

- Compilateur C avec support MPI (gcc avec mpicc, clang avec mpicc)
- **MPI** : OpenMPI ou MPICH (bibliothèque MPI 3.0+)
- Bibliothèques pour la visualisation (optionnel) :
  - Bibliothèque X11 pour l'affichage graphique
  - Serveur X11 actif

### Installation des dépendances (Ubuntu/Debian)

```bash
# Pour la compilation avec MPI
sudo apt install build-essential
sudo apt install libopenmpi-dev openmpi-bin
# ou
sudo apt install libmpich-dev mpich

# Pour la visualisation graphique (optionnel)
sudo apt install libx11-dev
```

### Installation des dépendances (Fedora/RHEL)

```bash
# Pour la compilation avec MPI
sudo yum install gcc
sudo yum install openmpi-devel
# ou
sudo yum install mpich-devel

# Pour la visualisation graphique (optionnel)
sudo yum install libX11-devel
```

## Compilation

Le projet utilise un Makefile pour simplifier la compilation :

```bash
# Compiler tous les exécutables (séquentiel, parallèle, visualisation)
make

# Compiler uniquement la version séquentielle
make gen_lab

# Compiler uniquement la version parallèle
make gen_lab-parallel

# Compiler uniquement l'outil de visualisation
make chemin_lab
```

### Nettoyage

```bash
# Supprimer les exécutables et fichiers générés
make clean
```

## Utilisation

### Version séquentielle

```bash
./gen_lab [nbilots] [N] [M]
```

### Version parallèle

```bash
mpirun -np <nombre_processus> ./gen_lab-parallel [nbilots] [N] [M]
```

### Paramètres

- `nbilots` : Nombre d'îlots initiaux (défaut : 20)
- `N` : Hauteur du labyrinthe en nombre de lignes (défaut : 400)
- `M` : Largeur du labyrinthe en nombre de colonnes (défaut : 600)

**Note importante :** Pour la version parallèle, la hauteur `N` doit être divisible par le nombre de processus pour une distribution équilibrée des lignes.

### Exemples d'exécution

```bash
# Paramètres par défaut (20 îlots, 400×600)
./gen_lab
./gen_lab-parallel

# Avec 2 processus MPI
mpirun -np 2 ./gen_lab-parallel 20 400 600

# Avec 4 processus (utiliser --oversubscribe si plus de processus que de cœurs)
mpirun -np 4 --oversubscribe ./gen_lab-parallel 20 400 600

# Labyrinthe plus grand avec plus d'îlots
mpirun -np 3 --oversubscribe ./gen_lab-parallel 30 800 1200

# Version séquentielle avec paramètres personnalisés
./gen_lab 25 500 800
```

Les labyrinthes générés sont sauvegardés dans le fichier `laby.lab` (format binaire).

## Visualisation du labyrinthe

Après génération, le labyrinthe est sauvegardé dans `laby.lab`. Pour visualiser et résoudre le labyrinthe :

```bash
# Visualisation avec recherche du chemin le plus court
./chemin_lab

# Visualisation avec recherche aléatoire de chemin
./chemin_lab 1
```

**Fonctionnalités de visualisation :**
- Affichage graphique du labyrinthe (murs en noir, chemins en blanc)
- Recherche aléatoire de chemin (option en ligne de commande)
- Recherche du chemin le plus court via algorithme d'inondation
- Navigation interactive (appuyez sur 'q' pour quitter)

**Note :** La visualisation nécessite un serveur X11 actif. Sur les systèmes headless ou sans interface graphique, utilisez X11 forwarding via SSH (`ssh -X`) ou une solution alternative.

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

Les résultats sont sauvegardés dans `mesures_results.txt`.

## Architecture et algorithme

### Algorithme de génération

L'algorithme procède par itérations successives :

1. **Initialisation** : Création d'une grille vide avec murs périphériques
2. **Placement d'îlots** : Ajout aléatoire d'îlots initiaux (obstacles isolés)
3. **Identification des cases constructibles** : Une case est constructible si elle est vide et a exactement un voisin mur parmi ses 8 voisins
4. **Construction itérative** :
   - Sélection aléatoire d'une case constructible parmi toutes les cases disponibles
   - Transformation de cette case en mur
   - Mise à jour des 8 voisins (ajout/retrait de la liste des constructibles)
   - Répétition jusqu'à ce qu'il n'y ait plus de cases constructibles

### Parallélisation MPI

#### Décomposition de domaine 1D

Le labyrinthe de taille `N × M` est divisé en bandes horizontales :
- Chaque processus `i` gère `N_loc = N / size` lignes consécutives
- Chaque processus maintient deux lignes fantômes (ghost rows) pour la cohérence des frontières

#### Communication MPI

L'échange de lignes fantômes utilise `MPI_Sendrecv` pour garantir l'absence de deadlock et maintenir la cohérence des données aux frontières entre processus voisins.

#### Synchronisation globale

À chaque itération, une synchronisation globale calcule le nombre total de cases constructibles via `MPI_Allreduce` pour déterminer la fin de l'algorithme.

## Structure du projet

```
labyrinthe/
├── README.md                  # Ce fichier
├── Makefile                   # Fichier de compilation
├── gen_lab.c                  # Version séquentielle
├── gen_lab-parallel.c         # Version parallèle avec MPI
├── chemin_lab.c               # Visualisation et résolution
├── graph.c                    # Bibliothèque graphique X11
├── graph.h                    # En-tête bibliothèque graphique
├── mesures_performances.sh    # Script de mesure de performances
├── mesures_results.txt        # Résultats des mesures (généré)
├── laby.lab                   # Labyrinthe généré (format binaire, généré)
├── rapport_projet_lab.tex     # Rapport LaTeX du projet
└── rapport_projet_lab.pdf     # Rapport compilé
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

## Auteur

**Junior Koudogbo**

Projet réalisé dans le cadre du cours de Programmation parallèle et distribuée, Université de Versailles - Saint Quentin en Yvelines, 2025.

## Références

- [MPI Standard](https://www.mpi-forum.org/)
- [OpenMPI Documentation](https://www.open-mpi.org/doc/)
- [MPICH Documentation](https://www.mpich.org/documentation/)

## Licence

Ce projet est fourni à des fins éducatives dans le cadre d'un cours universitaire.
