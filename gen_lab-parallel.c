/*
 * gen_lab-parallel.c
 * Génération de labyrinthe parallèle avec MPI
 * Stratégie : Décomposition de domaine 1D (bandes de lignes) avec échanges de lignes fantômes.
 */

 #include <fcntl.h>
 #include <mpi.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <time.h>
 #include <unistd.h>
 #include <string.h>
 
 /* * L'affichage X11 est désactivé (#undef AFFICHE) pour la version parallèle MPI
  * car la gestion de fenêtre partagée est complexe et inefficace en HPC.
  * La visualisation se fera a posteriori avec ./chemin_lab
  */
 // #define AFFICHE
 
 /* nombre de cases constructibles minimal */
 #define CONSMIN 10
 /* probabilité qu'une case du bord ne soit pas constructible */
 #define PROBPASCONS 10
 /* nombre d'ilots par defaut */
 #define NBILOTS 20
 
 /* Fonction auxiliaire : Échange des lignes fantômes (Halo exchange) */
 void update_ghosts(int rows, int cols, int l[rows][cols], int rank, int size) {
     MPI_Status status;
     
     // Voisins
     int up_neighbor = (rank == 0) ? MPI_PROC_NULL : rank - 1;
     int down_neighbor = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;
 
     // Échange vers le HAUT : j'envoie ma ligne 1, je reçois dans ma ligne 0
     MPI_Sendrecv(&l[1][0], cols, MPI_INT, up_neighbor, 0,
                  &l[0][0], cols, MPI_INT, up_neighbor, 0,
                  MPI_COMM_WORLD, &status);
 
     // Échange vers le BAS : j'envoie ma ligne rows-2, je reçois dans ma ligne rows-1
     MPI_Sendrecv(&l[rows-2][0], cols, MPI_INT, down_neighbor, 0,
                  &l[rows-1][0], cols, MPI_INT, down_neighbor, 0,
                  MPI_COMM_WORLD, &status);
 }
 
 /* fonction estconstructible : renvoie vrai si la case (i,j) est constructible */
 static int estconstructible(int rows, int M, int l[rows][M], int i, int j) {
   if (l[i][j] == 0)
     return 0;
   else if ((l[i - 1][j] == 0 && l[i][j + 1] && l[i][j - 1] && l[i + 1][j - 1] &&
             l[i + 1][j] && l[i + 1][j + 1]) ||
            (l[i + 1][j] == 0 && l[i][j + 1] && l[i][j - 1] && l[i - 1][j - 1] &&
             l[i - 1][j] && l[i - 1][j + 1]) ||
            (l[i][j - 1] == 0 && l[i + 1][j] && l[i - 1][j] && l[i - 1][j + 1] &&
             l[i][j + 1] && l[i + 1][j + 1]) ||
            (l[i][j + 1] == 0 && l[i + 1][j] && l[i - 1][j] && l[i - 1][j - 1] &&
             l[i][j - 1] && l[i + 1][j - 1]))
     return 1;
   else
     return 0;
 }
 
 int main(int argc, char* argv[]) {
   MPI_Init(&argc, &argv);
 
   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
 
   int nbilots = NBILOTS;
   if (argc > 1) nbilots = strtoull(argv[1], 0, 0);
 
   /* taille du labyrinthe global */
   size_t N = 400;
   if (argc > 2) N = strtoull(argv[2], 0, 0);
   size_t M = 600;
   if (argc > 3) M = strtoull(argv[3], 0, 0);
 
   /* Calcul de la taille locale (suppose N divisible par size pour simplifier) */
   int N_loc = N / size; 
   int start_row = rank * N_loc; 
   
   /* Allocation locale : N_loc lignes + 2 lignes fantômes (0 et N_loc+1) */
   int local_rows = N_loc + 2;
   int(*l)[M] = malloc(sizeof(int[local_rows][M]));
 
   /* Initialisation de la graine aléatoire différente par processus */
   srand(time(NULL) + rank * 1000);
 
   double demarrage = MPI_Wtime();
 
   /* 1. INITIALISATION LOCALE */
   for (int i = 1; i <= N_loc; i++) {
     int global_i = start_row + (i - 1);
     for (int j = 0; j < M; j++) {
       if (global_i == 0 || global_i == N - 1 || j == 0 || j == M - 1) {
         l[i][j] = 0; /* mur */
       } else {
         l[i][j] = 1; /* vide */
       }
     }
   }
   // Init fantômes à 1 par précaution
   for(int j=0; j<M; j++) { l[0][j] = 1; l[local_rows-1][j] = 1; }
 
   /* 2. PLACEMENT DES ILOTS */
   int local_ilots = nbilots / size; 
   if (rank == 0) local_ilots += nbilots % size;
 
   for (int k = 0; k < local_ilots; k++) {
     int i_loc = rand() % (N_loc);
     int global_i = start_row + i_loc;
     
     if (global_i > 1 && global_i < N - 2) {
        int j = rand() % (M - 4) + 2;
        l[i_loc + 1][j] = 0; // +1 car l'index 0 est fantôme
     }
   }
 
   update_ghosts(local_rows, M, l, rank, size);
 
   /* 3. CALCUL INITIAL DES CONSTRUCTIBLES */
   int nbcons = 0;
   for (int i = 1; i <= N_loc; i++) {
     for (int j = 1; j < M - 1; j++) {
       if (estconstructible(local_rows, M, l, i, j)) {
         l[i][j] = -1;
         nbcons++;
       }
     }
   }
 
   /* Suppression aléatoire sur les bords */
   for (int i = 1; i <= N_loc; i++) {
       if (l[i][1] == -1 && (rand() % PROBPASCONS) && nbcons > (CONSMIN * 2)) {
           l[i][1] = 1; nbcons--;
       }
       if (l[i][M - 2] == -1 && (rand() % PROBPASCONS) && nbcons > (CONSMIN * 2)) {
           l[i][M - 2] = 1; nbcons--;
       }
   }
   if (rank == 0) {
       for (int j = 1; j < M - 1; j++) {
         if (l[1][j] == -1 && (rand() % PROBPASCONS) && nbcons > CONSMIN) {
             l[1][j] = 1; nbcons--;
         }
       }
   }
   if (rank == size - 1) {
       for (int j = 1; j < M - 1; j++) {
         if (l[N_loc][j] == -1 && (rand() % PROBPASCONS) && nbcons > CONSMIN) {
             l[N_loc][j] = 1; nbcons--;
         }
       }
   }
 
   /* 4. BOUCLE PRINCIPALE */
   int global_nbcons = 1;
   while (1) {
     MPI_Allreduce(&nbcons, &global_nbcons, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
     if (global_nbcons == 0) break;
 
     if (nbcons > 0) {
         int r = 1 + rand() % nbcons;
         // CORRECTION ICI : Initialisation pour éviter le warning
         int i = 0, j = 0, found = 0;
         
         for (i = 1; i <= N_loc; i++) {
             for (j = 1; j < M - 1; j++) {
                 if (l[i][j] == -1) {
                     if (--r == 0) {
                         found = 1;
                         break;
                     }
                 }
             }
             if (found) break;
         }
 
         // CORRECTION ICI : Sécurité avec if(found)
         if (found) {
             l[i][j] = 0;
             nbcons--;
 
             for (int ii = i - 1; ii <= i + 1; ++ii) {
                 for (int jj = j - 1; jj <= j + 1; ++jj) {
                     if (ii >= 1 && ii <= N_loc) {
                         if (l[ii][jj] == 1 && estconstructible(local_rows, M, l, ii, jj)) {
                             nbcons++;
                             l[ii][jj] = -1;
                         } else if (l[ii][jj] == -1 && !estconstructible(local_rows, M, l, ii, jj)) {
                             nbcons--;
                             l[ii][jj] = 1;
                         }
                     }
                 }
             }
         }
     }
 
     update_ghosts(local_rows, M, l, rank, size);
 
     // Re-check des frontières après échange
     int boundary_rows[] = {1, N_loc};
     for (int k=0; k<2; k++) {
         int row = boundary_rows[k];
         if (row < 1 || row > N_loc) continue;
 
         for (int j = 1; j < M - 1; j++) {
             if (l[row][j] == 1 && estconstructible(local_rows, M, l, row, j)) {
                 nbcons++;
                 l[row][j] = -1;
             } else if (l[row][j] == -1 && !estconstructible(local_rows, M, l, row, j)) {
                 nbcons--;
                 l[row][j] = 1;
             }
         }
     }
   }
 
   double fin = MPI_Wtime();
 
   /* 5. RECONSTITUTION ET SAUVEGARDE */
   int *full_grid = NULL;
   if (rank == 0) {
       full_grid = malloc(N * M * sizeof(int));
   }
 
   // Préparation buffer sans les lignes fantômes
   int *send_buffer = malloc(N_loc * M * sizeof(int));
   for(int i=0; i<N_loc; i++) {
       memcpy(&send_buffer[i*M], &l[i+1][0], M * sizeof(int));
   }
 
   MPI_Gather(send_buffer, N_loc * M, MPI_INT, 
              full_grid, N_loc * M, MPI_INT, 
              0, MPI_COMM_WORLD);
 
   if (rank == 0) {
       printf("Temps de génération MPI : %lg s\n", fin - demarrage);
       
       int f = open("laby.lab", O_WRONLY | O_CREAT, 0644);
       int x = N;
       write(f, &x, sizeof(int));
       x = M;
       write(f, &x, sizeof(int));
       write(f, full_grid, N * M * sizeof(int));
       close(f);
       
       free(full_grid);
   }
 
   free(send_buffer);
   free(l);
   
   MPI_Finalize();
   return EXIT_SUCCESS;
 }