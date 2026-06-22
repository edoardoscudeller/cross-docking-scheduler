#!/usr/bin/env bash
# ===========================================================================
# run_all.sh
# Lancia CD_Test su tutte le istanze della v0.1, raggruppate per scenario.
# Output: results_v0.1.txt (riscritto ad ogni esecuzione)
# Uso: bash run_all.sh (dalla root cross-docking-scheduler/)
#
# Compatibile con Bash 3.2 (macOS): non usa mapfile.
# ===========================================================================

BINARY="./SourceFiles/CD_Test.exe"
INSTANCES_DIR="./Instances"
OUTPUT="results_v0.1.txt"

# Istanze previste:
# - 8x5 con seed 101, 111, 121
# - 12x8 con seed 102, 112, 122
# - 20x13 con seed 103, 113, 123
# - 40x20 con seed 201
# - 60x30 con seed 202
# - 80x40 con seed 203
# - 100x50 con seed 204
# - 120x60 con seed 205
# - 200x100 con seed 212
# - 300x150 con seed 213

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

SCENARIOS=(uniform sparse clustered asymmetric congested urgent mixed)

> "$OUTPUT"

if [ -x "./SourceFiles/CD_Test.exe" ]; then
    BINARY="./SourceFiles/CD_Test.exe"
elif [ -x "./SourceFiles/CD_Test" ]; then
    BINARY="./SourceFiles/CD_Test"
else
    echo "Errore: nessun binario trovato (né CD_Test.exe né CD_Test)."
    exit 1
fi

if [ ! -d "$INSTANCES_DIR" ]; then
  echo "Errore: cartella $INSTANCES_DIR non trovata."
  exit 1
fi

for SCENARIO in "${SCENARIOS[@]}"; do
  echo ">>> Scenario: $SCENARIO"

  declare -a arr_name arr_seed arr_n arr_m arr_din arr_dout
  declare -a arr_lb arr_makespan arr_gap arr_cpu_s arr_cpu_min
  declare -a arr_release arr_inbound arr_outbound

  idx=0

  for SIZE in "${SIZES[@]}"; do
    N=$(echo "$SIZE" | awk '{print $1}')
    M=$(echo "$SIZE" | awk '{print $2}')
    SEED=$(echo "$SIZE" | awk '{print $3}')

    INSTANCE="${INSTANCES_DIR}/cd_n$(printf "%04d" "$N")_m$(printf "%04d" "$M")_${SCENARIO}_s${SEED}.dzn"

    if [ ! -f "$INSTANCE" ]; then
      echo " [SKIP] non trovata: $INSTANCE"
      continue
    fi

    BASENAME=$(basename "$INSTANCE" .dzn)
    echo " Esecuzione: $BASENAME"

    RAW=$("$BINARY" "$INSTANCE" 2>/dev/null)
    STATUS=$?

    LB=""
    MAKESPAN=""
    GAP=""
    CPU_S=""
    CPU_MIN=""
    DIN=""
    DOUT=""
    RELEASE=""
    INBOUND=""
    OUTBOUND=""

    if [ $STATUS -eq 0 ] && [ -n "$RAW" ]; then
      LB=$(echo "$RAW" | awk -F': ' '/^Lower Bound[[:space:]]*: / {print $2}' | head -n 1)
      MAKESPAN=$(echo "$RAW" | awk -F': ' '/^Makespan[[:space:]]*: / {print $2}' | head -n 1)
      GAP=$(echo "$RAW" | sed -n 's/^Gap (%)[[:space:]]*:[[:space:]]*\([0-9.][0-9.]*\)%$/\1/p' | head -n 1)
      CPU_S=$(echo "$RAW" | awk -F': ' '/^CPU time[[:space:]]*: / {print $2}' | sed 's/ s$//' | head -n 1)

      DIN=$(echo "$RAW" | sed -n 's/^Doors[[:space:]]*:[[:space:]]*in=\([0-9][0-9]*\)[[:space:]]*out=\([0-9][0-9]*\)$/\1/p' | head -n 1)
      DOUT=$(echo "$RAW" | sed -n 's/^Doors[[:space:]]*:[[:space:]]*in=\([0-9][0-9]*\)[[:space:]]*out=\([0-9][0-9]*\)$/\2/p' | head -n 1)

      if [ -n "$CPU_S" ]; then
        CPU_MIN=$(awk -v t="$CPU_S" 'BEGIN { printf "%.6f", t/60.0 }')
      fi

      if [ "$N" -le 20 ]; then
        RELEASE=$(echo "$RAW" | awk '/^ReleaseTime/ {sub(/^[^=]*=[[:space:]]*/, "", $0); print; exit}')
        INBOUND=$(echo "$RAW" | awk 'tolower($0) ~ /^inbound[[:space:]]+sequence:/ {sub(/^[^:]*:[[:space:]]*/, "", $0); print; exit}')
        OUTBOUND=$(echo "$RAW" | awk 'tolower($0) ~ /^outbound[[:space:]]+sequence:/ {sub(/^[^:]*:[[:space:]]*/, "", $0); print; exit}')
        [ -z "$RELEASE" ] && RELEASE="(n<=20: not found)"
        [ -z "$INBOUND" ] && INBOUND="(n<=20: not found)"
        [ -z "$OUTBOUND" ] && OUTBOUND="(n<=20: not found)"
      else
        RELEASE="(n>20: not printed)"
        INBOUND="(n>20: not printed)"
        OUTBOUND="(n>20: not printed)"
      fi
    else
      LB="(execution failed)"
      MAKESPAN="(execution failed)"
      GAP="(execution failed)"
      CPU_S="(execution failed)"
      CPU_MIN="(execution failed)"
      DIN="(execution failed)"
      DOUT="(execution failed)"

      if [ "$N" -le 20 ]; then
        RELEASE="(execution failed)"
        INBOUND="(execution failed)"
        OUTBOUND="(execution failed)"
      else
        RELEASE="(n>20: not printed)"
        INBOUND="(n>20: not printed)"
        OUTBOUND="(n>20: not printed)"
      fi
    fi

    arr_name[$idx]="$BASENAME"
    arr_seed[$idx]="$SEED"
    arr_n[$idx]="$N"
    arr_m[$idx]="$M"
    arr_din[$idx]="$DIN"
    arr_dout[$idx]="$DOUT"
    arr_lb[$idx]="$LB"
    arr_makespan[$idx]="$MAKESPAN"
    arr_gap[$idx]="$GAP"
    arr_cpu_s[$idx]="$CPU_S"
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
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_name[$i]}"
    done
    echo ""

    echo "Seed"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_seed[$i]}"
    done
    echo ""

    echo "N inbound"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_n[$i]}"
    done
    echo ""

    echo "M outbound"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_m[$i]}"
    done
    echo ""

    echo "Inbound doors"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_din[$i]}"
    done
    echo ""

    echo "Outbound doors"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_dout[$i]}"
    done
    echo ""

    echo "Lower Bound"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_lb[$i]}"
    done
    echo ""

    echo "Makespan"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_makespan[$i]}"
    done
    echo ""

    echo "Gap (%)"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_gap[$i]}"
    done
    echo ""

    echo "CPU Time (s)"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_cpu_s[$i]}"
    done
    echo ""

    echo "CPU Time (min)"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_cpu_min[$i]}"
    done
    echo ""

    echo "Release time"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_release[$i]}"
    done
    echo ""

    echo "Inbound sequence"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_inbound[$i]}"
    done
    echo ""

    echo "Outbound sequence"
    for ((i=0; i<COUNT; i++)); do
      echo "${arr_outbound[$i]}"
    done
    echo ""
  } >> "$OUTPUT"

  unset arr_name arr_seed arr_n arr_m arr_din arr_dout
  unset arr_lb arr_makespan arr_gap arr_cpu_s arr_cpu_min
  unset arr_release arr_inbound arr_outbound
done

echo "Fatto. Risultati salvati in: $OUTPUT"