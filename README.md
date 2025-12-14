# Génération de Labyrinthe - Parallélisation MPI

Projet de parallélisation de la génération de labyrinthe en utilisant MPI (Message Passing Interface).

## Description

Ce projet implémente un algorithme de génération de labyrinthe avec deux versions :
- **Version séquentielle** (`gen_lab.c`) : Génération séquentielle de labyrinthe
- **Version parallèle** (`gen_lab-parallel.c`) : Génération parallèle utilisant MPI avec décomposition de domaine 1D

## Structure du projet

```
.
├── gen_lab.c              # Version séquentielle
├── gen_lab-parallel.c     # Version parallèle MPI
├── chemin_lab.c           # Visualisation du labyrinthe
├── graph.c, graph.h       # Bibliothèque graphique
├── Makefile               # Fichier de compilation
├── mesures_performances.sh # Script de mesures de performances
├── mesures_results.txt    # Résultats des mesures
├── rapport_projet_lab.tex # Rapport LaTeX du projet
└── rapport_projet_lab.pdf # Rapport compilé
```

## Compilation

```bash
make
```

Cela compile les trois exécutables :
- `gen_lab` : version séquentielle
- `gen_lab-parallel` : version parallèle MPI
- `chemin_lab` : visualisation

## Utilisation

### Génération séquentielle

```bash
./gen_lab [nbilots] [N] [M]
```

Paramètres :
- `nbilots` : nombre d'îlots (défaut: 20)
- `N` : hauteur du labyrinthe (défaut: 400)
- `M` : largeur du labyrinthe (défaut: 600)

### Génération parallèle

```bash
# Avec 2 processus
mpirun -np 2 ./gen_lab-parallel [nbilots] [N] [M]

# Avec 3 ou 4 processus (oversubscribe)
mpirun -np 3 --oversubscribe ./gen_lab-parallel [nbilots] [N] [M]
mpirun -np 4 --oversubscribe ./gen_lab-parallel [nbilots] [N] [M]
```

### Visualisation

```bash
./chemin_lab
```

## Mesures de performances

Pour effectuer les mesures de performances (scalabilité forte et faible) :

```bash
./mesures_performances.sh
```

Les résultats sont sauvegardés dans `mesures_results.txt`.

## Résultats

Le rapport complet (`rapport_projet_lab.pdf`) présente une étude détaillée des performances avec :
- **Scalabilité forte** : Speedup de 3.52 (2 proc), 3.61 (3 proc), 2.69 (4 proc)
- **Scalabilité faible** : Efficacité de 64.4% (2 proc), 35.1% (3 proc), 17.3% (4 proc)

## Auteur

Junior Koudogbo

## Licence

Ce projet est fourni à des fins éducatives.

