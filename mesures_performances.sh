#!/bin/bash

# Script pour effectuer les mesures de performances
# Scalabilité forte et faible pour le projet Labyrinthe

OUTPUT_FILE="mesures_results.txt"

echo "=== MESURES DE PERFORMANCES - PROJET LABYRINTHE ===" > $OUTPUT_FILE
echo "" >> $OUTPUT_FILE

# Paramètres par défaut
NILOTS=20
NEXEC=3

# Scalabilité forte : taille fixe (valeurs par défaut 400x600)
echo "=== SCALABILITÉ FORTE (taille fixe: valeurs par défaut) ===" >> $OUTPUT_FILE
echo "Processus | Exécution | Temps (s)" >> $OUTPUT_FILE
echo "-----------------------------------" >> $OUTPUT_FILE

for NP in 1 2 3 4; do
    echo "Mesure avec $NP processus (scalabilité forte)..."
    for EXEC in $(seq 1 $NEXEC); do
        if [ $NP -eq 1 ]; then
            # Version séquentielle
            TIME=$(./gen_lab 2>&1 | grep "Temps" | sed 's/.*: \([0-9.]*\) s/\1/')
        elif [ $NP -eq 3 ] || [ $NP -eq 4 ]; then
            # Utiliser --oversubscribe pour 3 et 4 processus
            TIME=$(mpirun -np $NP --oversubscribe ./gen_lab-parallel 2>&1 | grep "Temps" | sed 's/.*: \([0-9.]*\) s/\1/')
        else
            # Version parallèle normale pour 2 processus
            TIME=$(mpirun -np $NP ./gen_lab-parallel 2>&1 | grep "Temps" | sed 's/.*: \([0-9.]*\) s/\1/')
        fi
        echo "$NP | $EXEC | $TIME" >> $OUTPUT_FILE
        echo "  Exécution $EXEC/$NEXEC: $TIME s"
    done
done

echo "" >> $OUTPUT_FILE
echo "=== SCALABILITÉ FAIBLE (taille proportionnelle) ===" >> $OUTPUT_FILE
echo "Processus | Dimensions | Exécution | Temps (s)" >> $OUTPUT_FILE
echo "------------------------------------------------" >> $OUTPUT_FILE

# Scalabilité faible : taille proportionnelle
# 1 processus : 400x600 (base)
# 2 processus : 565x848 (surface x2)
# 3 processus : 692x1038 (surface x3)
# 4 processus : 800x1200 (surface x4)

for NP in 1 2 3 4; do
    case $NP in
        1)
            N=400
            M=600
            ;;
        2)
            N=565
            M=848
            ;;
        3)
            N=692
            M=1038
            ;;
        4)
            N=800
            M=1200
            ;;
    esac
    
    echo "Mesure avec $NP processus (scalabilité faible, dimensions: ${N}x${M})..."
    for EXEC in $(seq 1 $NEXEC); do
        if [ $NP -eq 1 ]; then
            TIME=$(./gen_lab $NILOTS $N $M 2>&1 | grep "Temps" | sed 's/.*: \([0-9.]*\) s/\1/')
        elif [ $NP -eq 3 ] || [ $NP -eq 4 ]; then
            TIME=$(mpirun -np $NP --oversubscribe ./gen_lab-parallel $NILOTS $N $M 2>&1 | grep "Temps" | sed 's/.*: \([0-9.]*\) s/\1/')
        else
            TIME=$(mpirun -np $NP ./gen_lab-parallel $NILOTS $N $M 2>&1 | grep "Temps" | sed 's/.*: \([0-9.]*\) s/\1/')
        fi
        echo "$NP | ${N}x${M} | $EXEC | $TIME" >> $OUTPUT_FILE
        echo "  Exécution $EXEC/$NEXEC: $TIME s"
    done
done

echo "" >> $OUTPUT_FILE
echo "=== MESURES TERMINÉES ===" >> $OUTPUT_FILE
echo ""
echo "Résultats sauvegardés dans $OUTPUT_FILE"
