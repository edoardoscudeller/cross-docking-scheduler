#!/bin/bash
# run_instances.sh
# Genera tutte le istanze del progetto cross-docking-scheduler v0.1
# Uso: bash run_instances.sh
# Le istanze verranno salvate nella cartella Instances/

cd Instances || exit 1

SCENARIOS=(uniform sparse clustered asymmetric congested urgent mixed)

SIZES=(
  "8 5 101"
  "8 5 111"
  "8 5 121"
  "12 8 102"
  "12 8 112"
  "12 8 122"
  "20 13 103"
  "20 13 113"
  "20 13 123"
  "40 20 201"
  "60 30 202"
  "80 40 203"
  "100 50 204"
  "120 60 205"
  "200 100 212"
  "300 150 213"
)

for SCENARIO in "${SCENARIOS[@]}"; do
  echo ">>> Scenario: $SCENARIO"

  for SIZE in "${SIZES[@]}"; do
    N=$(echo "$SIZE" | awk '{print $1}')
    M=$(echo "$SIZE" | awk '{print $2}')
    SEED=$(echo "$SIZE" | awk '{print $3}')

    echo "  Genero: n=$N m=$M seed=$SEED scenario=$SCENARIO"
    python3 generate.py "$N" "$M" "$SEED" "$SCENARIO"
  done

  echo ""
done

echo "Tutte le istanze generate con successo."