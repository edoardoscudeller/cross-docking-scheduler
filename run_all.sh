#!/usr/bin/env bash
# ===========================================================================
# run_all.sh
# Lancia CD_Test su tutte le istanze, raggruppate per scenario.
# Output: results.txt (riscritto ad ogni esecuzione)
# Uso:    bash run_all.sh   (dalla root cross-docking-scheduler/)
# ===========================================================================

#!/bin/bash
# run_all.sh  — versione aggiornata
# Stampa per ogni istanza: nome, seed, n, m, makespan, CPU(s), CPU(min),
# release time (solo se n<=20), inbound sequence (n<=20), outbound sequence (n<=20)
# Risultati organizzati per scenario, in blocchi.
# Uso: bash run_all.sh   (dalla root cross-docking-scheduler/)

BINARY="./SourceFiles/CD_Test"
INSTANCES_DIR="./Instances"
OUTPUT="results_v02.txt"

SIZES=(
  "8 5 101"
  "12 8 102"
  "20 13 103"
  "40 20 201"
  "60 30 202"
  "80 40 203"
  "100 50 204"
  "120 60 205"
  "200 100 301"
  "300 150 302"
  "400 200 303"
  "500 250 304"
  "600 300 305"
  "40 20 211"
  "100 50 212"
  "300 150 213"
  "600 300 214"
)

SCENARIOS=(uniform sparse clustered asymmetric congested urgent mixed)

> "$OUTPUT"

for SCENARIO in "${SCENARIOS[@]}"; do
  echo ">>> Scenario: $SCENARIO"

  declare -a arr_name arr_n arr_m arr_seed arr_makespan arr_cpu arr_cpu_min \
             arr_release arr_inbound arr_outbound
  idx=0

  for SIZE in "${SIZES[@]}"; do
    N=$(echo    $SIZE | awk '{print $1}')
    M=$(echo    $SIZE | awk '{print $2}')
    SEED=$(echo $SIZE | awk '{print $3}')
    INSTANCE="${INSTANCES_DIR}/cd_n$(printf "%04d" $N)_m$(printf "%04d" $M)_${SCENARIO}_s${SEED}.dzn"

    if [ ! -f "$INSTANCE" ]; then
      echo "  [SKIP] non trovata: $INSTANCE"
      continue
    fi

    BASENAME=$(basename "$INSTANCE" .dzn)
    echo "  Esecuzione: $BASENAME"
    RAW=$("$BINARY" "$INSTANCE" 2>/dev/null)

    # Parsing makespan e CPU dall'output del binario
    MAKESPAN=$(echo "$RAW" | grep -E "^Makespan\s*:" | awk '{print $3}')
    CPU=$(echo      "$RAW" | grep -E "^CPU time\s*:" | awk '{print $3}')

    # CPU in minuti (usa awk per divisione float)
    if [ -n "$CPU" ]; then
      CPU_MIN=$(awk "BEGIN {printf \"%.6f\", $CPU / 60}")
    else
      CPU_MIN=""
    fi

    # Release time: letto direttamente dal file .dzn (sempre disponibile)
    # ma mostrato solo se N <= 20
    if [ "$N" -le 20 ]; then
      RELEASE=$(grep -A1 "ReleaseTime" "$INSTANCE" | tail -1 \
                | sed 's/.*\[//;s/\].*//' \
                | tr -d ' ')
      RELEASE="[${RELEASE}]"
    else
      RELEASE="(n>20: not printed)"
    fi

    # Inbound / Outbound sequence: solo se N <= 20 (stampate dal binario)
    if [ "$N" -le 20 ]; then
      INBOUND=$(echo  "$RAW" | grep "^Inbound  sequence:" | sed 's/^Inbound  sequence: //')
      OUTBOUND=$(echo "$RAW" | grep "^Outbound sequence:" | sed 's/^Outbound sequence: //')
      [ -z "$INBOUND"  ] && INBOUND="(not found in output)"
      [ -z "$OUTBOUND" ] && OUTBOUND="(not found in output)"
    else
      INBOUND="(n>20: not printed)"
      OUTBOUND="(n>20: not printed)"
    fi

    arr_name[$idx]="$BASENAME"
    arr_n[$idx]="$N"
    arr_m[$idx]="$M"
    arr_seed[$idx]="$SEED"
    arr_makespan[$idx]="$MAKESPAN"
    arr_cpu[$idx]="$CPU"
    arr_cpu_min[$idx]="$CPU_MIN"
    arr_release[$idx]="$RELEASE"
    arr_inbound[$idx]="$INBOUND"
    arr_outbound[$idx]="$OUTBOUND"
    idx=$((idx + 1))
  done

  COUNT=$idx
  {
    echo "========================================"
    echo "SCENARIO: $SCENARIO"
    echo "========================================"
    echo ""
    echo "Instance name"
    for ((i=0; i<COUNT; i++)); do echo "${arr_name[$i]}";     done
    echo ""
    echo "Seed"
    for ((i=0; i<COUNT; i++)); do echo "${arr_seed[$i]}";     done
    echo ""
    echo "n (inbound)"
    for ((i=0; i<COUNT; i++)); do echo "${arr_n[$i]}";        done
    echo ""
    echo "m (outbound)"
    for ((i=0; i<COUNT; i++)); do echo "${arr_m[$i]}";        done
    echo ""
    echo "Makespan"
    for ((i=0; i<COUNT; i++)); do echo "${arr_makespan[$i]}"; done
    echo ""
    echo "CPU Time (s)"
    for ((i=0; i<COUNT; i++)); do echo "${arr_cpu[$i]}";      done
    echo ""
    echo "CPU Time (min)"
    for ((i=0; i<COUNT; i++)); do echo "${arr_cpu_min[$i]}";  done
    echo ""
    echo "Release time"
    for ((i=0; i<COUNT; i++)); do echo "${arr_release[$i]}";  done
    echo ""
    echo "Inbound sequence"
    for ((i=0; i<COUNT; i++)); do echo "${arr_inbound[$i]}";  done
    echo ""
    echo "Outbound sequence"
    for ((i=0; i<COUNT; i++)); do echo "${arr_outbound[$i]}"; done
    echo ""
  } >> "$OUTPUT"

  unset arr_name arr_n arr_m arr_seed arr_makespan arr_cpu arr_cpu_min \
        arr_release arr_inbound arr_outbound
done

echo "Fatto. Risultati salvati in: $OUTPUT"
SCRIPT
