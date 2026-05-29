#!/usr/bin/env bash
# ===========================================================================
# run_all.sh
# Lancia CD_Test su tutte le istanze, raggruppate per scenario.
# Output: results_v05.txt (riscritto ad ogni esecuzione)
# Uso:    bash run_all.sh   (dalla root cross-docking-scheduler/)
# ===========================================================================

BINARY="./SourceFiles/CD_Test.exe"
INSTANCES_DIR="./Instances"
OUTPUT="results_v0.2.txt"

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

if [ ! -x "$BINARY" ]; then
  echo "Errore: binario $BINARY non trovato o non eseguibile."
  exit 1
fi

for SCENARIO in "${SCENARIOS[@]}"; do
  echo ">>> Scenario: $SCENARIO"

  declare -a arr_name arr_seed arr_n arr_m arr_din arr_dout
  declare -a arr_makespan arr_cpu_s arr_cpu_min
  declare -a arr_release arr_inbound arr_outbound

  idx=0

  for SIZE in "${SIZES[@]}"; do
    N=$(echo "$SIZE" | awk '{print $1}')
    M=$(echo "$SIZE" | awk '{print $2}')
    SEED=$(echo "$SIZE" | awk '{print $3}')

    INSTANCE="${INSTANCES_DIR}/cd_n$(printf "%04d" "$N")_m$(printf "%04d" "$M")_${SCENARIO}_s${SEED}.dzn"

    if [ ! -f "$INSTANCE" ]; then
      echo "  [SKIP] non trovata: $INSTANCE"
      continue
    fi

    BASENAME=$(basename "$INSTANCE" .dzn)
    echo "  Esecuzione: $BASENAME"

    RAW=$("$BINARY" "$INSTANCE" 2>/dev/null)
    STATUS=$?

    MAKESPAN=""
    CPU_S=""
    CPU_MIN=""
    DIN=""
    DOUT=""
    RELEASE=""
    INBOUND=""
    OUTBOUND=""

    if [ $STATUS -eq 0 ] && [ -n "$RAW" ]; then
      MAKESPAN=$(echo "$RAW" | awk -F': ' '/^Makespan : / {print $2}' | head -n 1)
      CPU_S=$(echo "$RAW" | awk -F': ' '/^CPU time : / {print $2}' | sed 's/ s$//' | head -n 1)

      DIN=$(echo "$RAW" | sed -n 's/^Doors[[:space:]]*:[[:space:]]*in=\([0-9][0-9]*\)[[:space:]]*out=\([0-9][0-9]*\)$/\1/p' | head -n 1)
      DOUT=$(echo "$RAW" | sed -n 's/^Doors[[:space:]]*:[[:space:]]*in=\([0-9][0-9]*\)[[:space:]]*out=\([0-9][0-9]*\)$/\2/p' | head -n 1)

      if [ -n "$CPU_S" ]; then
        CPU_MIN=$(awk -v t="$CPU_S" 'BEGIN { printf "%.6f", t/60.0 }')
      fi

      if [ "$N" -le 20 ]; then
        RELEASE=$(echo "$RAW" | awk '/^ReleaseTime/ {sub(/^[^=]*=[[:space:]]*/, "", $0); print; exit}')
        INBOUND=$(echo "$RAW" | sed -n 's/^Inbound  sequence: //p' | head -n 1)
        OUTBOUND=$(echo "$RAW" | sed -n 's/^Outbound sequence: //p' | head -n 1)

        [ -z "$RELEASE" ] && RELEASE="(n<=20: not found)"
        [ -z "$INBOUND" ] && INBOUND="(n<=20: not found)"
        [ -z "$OUTBOUND" ] && OUTBOUND="(n<=20: not found)"
      else
        RELEASE="(n>20: not printed)"
        INBOUND="(n>20: not printed)"
        OUTBOUND="(n>20: not printed)"
      fi
    else
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
    arr_makespan[$idx]="$MAKESPAN"
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
    for ((i=0; i<COUNT; i++)); do echo "${arr_name[$i]}"; done
    echo ""

    echo "Seed"
    for ((i=0; i<COUNT; i++)); do echo "${arr_seed[$i]}"; done
    echo ""

    echo "Inbound doors"
    for ((i=0; i<COUNT; i++)); do echo "${arr_din[$i]}"; done
    echo ""

    echo "Outbound doors"
    for ((i=0; i<COUNT; i++)); do echo "${arr_dout[$i]}"; done
    echo ""

    echo "Makespan"
    for ((i=0; i<COUNT; i++)); do echo "${arr_makespan[$i]}"; done
    echo ""

    echo "CPU Time (s)"
    for ((i=0; i<COUNT; i++)); do echo "${arr_cpu_s[$i]}"; done
    echo ""

    echo "CPU Time (min)"
    for ((i=0; i<COUNT; i++)); do echo "${arr_cpu_min[$i]}"; done
    echo ""

    echo "Release time"
    for ((i=0; i<COUNT; i++)); do echo "${arr_release[$i]}"; done
    echo ""

    echo "Inbound sequence"
    for ((i=0; i<COUNT; i++)); do echo "${arr_inbound[$i]}"; done
    echo ""

    echo "Outbound sequence"
    for ((i=0; i<COUNT; i++)); do echo "${arr_outbound[$i]}"; done
    echo ""

  } >> "$OUTPUT"

  unset arr_name arr_seed arr_n arr_m arr_din arr_dout
  unset arr_makespan arr_cpu_s arr_cpu_min
  unset arr_release arr_inbound arr_outbound
done

echo "Fatto. Risultati salvati in: $OUTPUT"