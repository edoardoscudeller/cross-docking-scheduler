#!/usr/bin/env bash
# =============================================================================
# run_all.sh — Esegue tutte le istanze .dzn e salva i risultati in un file TXT
# =============================================================================
# Uso (dalla root del progetto):
#   bash run_all.sh                  # esegue tutto, salva in results_v0.1.txt
#   bash run_all.sh uniform          # solo scenario uniform
#   bash run_all.sh sparse 42        # solo sparse con seed 42
#
# Prerequisiti:
#   - Binario compilato in ./SourceFiles/CD_Test (o CD_Test.exe su Windows)
#   - Istanze .dzn generate in ./Instances/ tramite run_instances.sh
#
# Output:
#   - results_v0.1.txt  (riscritto ad ogni esecuzione completa)
#   - run_all.log       (log dettagliato di ogni esecuzione)
#
# Naming istanze atteso (allineato a generate.py v0.2):
#   cd_n{N:04d}_m{M:04d}_{scenario}_d{density*100:02d}_s{seed}.dzn
#   es.: cd_n0008_m0005_uniform_d75_s42.dzn
#
# Taglie: 10 classi × 7 scenari × 5 seed = 350 istanze totali
# Unità temporale: 1 u.t. = 1 minuto
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Percorsi
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTANCES_DIR="${SCRIPT_DIR}/Instances"
OUTPUT="${SCRIPT_DIR}/results_v0.1.txt"
LOG_FILE="${SCRIPT_DIR}/run_all.log"

# ---------------------------------------------------------------------------
# Filtri CLI opzionali
# ---------------------------------------------------------------------------
FILTER_SCENARIO="${1:-}"
FILTER_SEED="${2:-}"

# ---------------------------------------------------------------------------
# Ricerca binario (Linux/macOS/Windows-MinGW)
# ---------------------------------------------------------------------------
BINARY=""
for candidate in \
    "${SCRIPT_DIR}/SourceFiles/CD_Test" \
    "${SCRIPT_DIR}/SourceFiles/CD_Test.exe" \
    "${SCRIPT_DIR}/SourceFiles/cd_test" \
    "${SCRIPT_DIR}/SourceFiles/cd_test.exe"; do
    if [ -x "$candidate" ]; then
        BINARY="$candidate"
        break
    fi
done

if [ -z "$BINARY" ]; then
    echo "[ERRORE] Nessun binario trovato in SourceFiles/."
    echo "         Compila prima con: cd SourceFiles && make"
    exit 1
fi

# ---------------------------------------------------------------------------
# Verifica cartella istanze
# ---------------------------------------------------------------------------
if [ ! -d "$INSTANCES_DIR" ]; then
    echo "[ERRORE] Cartella Instances/ non trovata in ${SCRIPT_DIR}."
    echo "         Genera le istanze prima con: bash run_instances.sh"
    exit 1
fi

# ---------------------------------------------------------------------------
# Taglie istanze: (n, m, d_in, d_out, density_tag)
# Allineate con run_instances.sh e generate.py v0.2
# ---------------------------------------------------------------------------
# Formato: "n m d_in d_out density_tag"
# density_tag = valore di default per scenario (es. 75 = 75%)
# ---------------------------------------------------------------------------
SIZES=(
    "8   5   4   4"
    "12  8   4   4"
    "20  13  5   5"
    "40  20  6   6"
    "60  30  6   6"
    "80  40  7   7"
    "100 50  10  10"
    "120 60  10  10"
    "200 100 20  20"
    "300 150 20  20"
)

SCENARIOS=(uniform sparse clustered asymmetric congested urgent mixed)
SEEDS=(42 101 202 314 999)

# Densità di default per scenario (deve matchare generate.py)
density_for_scenario() {
    case "$1" in
        uniform)    echo "75" ;;
        sparse)     echo "25" ;;
        clustered)  echo "35" ;;
        asymmetric) echo "50" ;;
        congested)  echo "75" ;;
        urgent)     echo "35" ;;
        mixed)      echo "50" ;;
        *)          echo "50" ;;
    esac
}

# ---------------------------------------------------------------------------
# Contatori globali
# ---------------------------------------------------------------------------
TOT_OK=0
TOT_SKIP=0
TOT_FAIL=0
START_TS=$(date +%s)

# ---------------------------------------------------------------------------
# Inizializzazione file output
# ---------------------------------------------------------------------------
{
    echo "============================================================"
    echo "  Cross-Docking Scheduler — Risultati v0.1"
    [ -n "${FILTER_SCENARIO}" ] && echo "  Filtro scenario : ${FILTER_SCENARIO}"
    [ -n "${FILTER_SEED}" ]     && echo "  Filtro seed     : ${FILTER_SEED}"
    echo "  Unità temp.     : 1 u.t. = 1 minuto"
    echo "============================================================"
    echo ""
} > "$OUTPUT"

echo "$(date '+%H:%M:%S') Inizio esecuzione — binary: ${BINARY}" > "$LOG_FILE"

# ---------------------------------------------------------------------------
# Loop principale: scenario → taglia → seed
# ---------------------------------------------------------------------------
for SCENARIO in "${SCENARIOS[@]}"; do

    # Applica filtro scenario
    if [ -n "${FILTER_SCENARIO}" ] && [ "${SCENARIO}" != "${FILTER_SCENARIO}" ]; then
        continue
    fi

    DENSITY_TAG="$(density_for_scenario "${SCENARIO}")"

    echo "============================================================" | tee -a "$OUTPUT"
    echo "SCENARIO: ${SCENARIO}  (densità default: ${DENSITY_TAG}%)"   | tee -a "$OUTPUT"
    echo "============================================================" | tee -a "$OUTPUT"
    printf "%-45s %8s %8s %8s %10s %10s\n" \
        "Istanza" "LB" "Makespan" "Gap(%)" "CPU(s)" "CPU(min)" | tee -a "$OUTPUT"
    printf "%s\n" "------------------------------------------------------------\
-----------------------------------------" | tee -a "$OUTPUT"

    for SIZE_ENTRY in "${SIZES[@]}"; do
        N=$(echo "$SIZE_ENTRY"  | awk '{print $1}')
        M=$(echo "$SIZE_ENTRY"  | awk '{print $2}')
        DIN=$(echo "$SIZE_ENTRY"  | awk '{print $3}')
        DOUT=$(echo "$SIZE_ENTRY" | awk '{print $4}')

        for SEED in "${SEEDS[@]}"; do

            # Applica filtro seed
            if [ -n "${FILTER_SEED}" ] && [ "${SEED}" != "${FILTER_SEED}" ]; then
                continue
            fi

            # Costruzione nome file (naming generate.py v0.2)
            N_PAD=$(printf "%04d" "$N")
            M_PAD=$(printf "%04d" "$M")
            INSTANCE="${INSTANCES_DIR}/cd_n${N_PAD}_m${M_PAD}_${SCENARIO}_d${DENSITY_TAG}_s${SEED}.dzn"
            BASENAME="cd_n${N_PAD}_m${M_PAD}_${SCENARIO}_d${DENSITY_TAG}_s${SEED}"

            # File non trovato → skip
            if [ ! -f "$INSTANCE" ]; then
                printf "  [SKIP] %-43s non trovata\n" "${BASENAME}" | tee -a "$OUTPUT"
                echo "$(date '+%H:%M:%S') SKIP  ${BASENAME}" >> "$LOG_FILE"
                TOT_SKIP=$((TOT_SKIP + 1))
                continue
            fi

            # Esecuzione binario
            echo "$(date '+%H:%M:%S') RUN   ${BASENAME}" >> "$LOG_FILE"
            RAW=$("$BINARY" "$INSTANCE" 2>/dev/null) || STATUS=$?
            STATUS=${STATUS:-0}

            if [ "$STATUS" -ne 0 ] || [ -z "$RAW" ]; then
                printf "  [ERR]  %-43s esecuzione fallita (exit=%d)\n" "${BASENAME}" "${STATUS}" | tee -a "$OUTPUT"
                echo "$(date '+%H:%M:%S') ERR   ${BASENAME} exit=${STATUS}" >> "$LOG_FILE"
                TOT_FAIL=$((TOT_FAIL + 1))
                continue
            fi

            # --- Parsing output binario ---
            LB=$(echo "$RAW"       | awk '/Lower Bound/{print $NF}')
            MAKESPAN=$(echo "$RAW" | awk '/Makespan/{print $NF}')
            GAP=$(echo "$RAW"      | awk '/Gap \(%\)/{gsub(/%/,"",$NF); print $NF}')
            CPU_S=$(echo "$RAW"    | awk '/CPU time/{print $(NF-1)}')

            # Valori mancanti → placeholder
            LB="${LB:----}"
            MAKESPAN="${MAKESPAN:----}"
            GAP="${GAP:----}"
            CPU_S="${CPU_S:----}"

            # CPU in minuti
            if [[ "$CPU_S" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                CPU_MIN=$(awk -v t="$CPU_S" 'BEGIN{printf "%.4f", t/60.0}')
            else
                CPU_MIN="---"
            fi

            # Stampa riga risultato
            printf "  %-45s %8s %8s %8s %10s %10s\n" \
                "${BASENAME}" "${LB}" "${MAKESPAN}" "${GAP}" "${CPU_S}" "${CPU_MIN}" \
                | tee -a "$OUTPUT"

            # Dettaglio sequenze per istanze piccole (n≤20)
            if [ "$N" -le 20 ]; then
                INBOUND=$(echo "$RAW"  | grep -i "Inbound.*Sequence"  | head -1)
                OUTBOUND=$(echo "$RAW" | grep -i "Outbound.*Sequence" | head -1)
                [ -n "$INBOUND"  ] && printf "    Inbound  seq: %s\n"  "${INBOUND}"  | tee -a "$OUTPUT"
                [ -n "$OUTBOUND" ] && printf "    Outbound seq: %s\n"  "${OUTBOUND}" | tee -a "$OUTPUT"
            fi

            echo "$(date '+%H:%M:%S') OK    ${BASENAME} LB=${LB} MS=${MAKESPAN} GAP=${GAP}% CPU=${CPU_S}s" >> "$LOG_FILE"
            TOT_OK=$((TOT_OK + 1))

        done  # seed
    done  # taglia

    echo "" | tee -a "$OUTPUT"

done  # scenario

# ---------------------------------------------------------------------------
# Riepilogo finale
# ---------------------------------------------------------------------------
END_TS=$(date +%s)
ELAPSED=$((END_TS - START_TS))

{
    echo "============================================================"
    echo " RIEPILOGO FINALE"
    echo "   Istanze eseguite OK : ${TOT_OK}"
    echo "   Saltate (not found) : ${TOT_SKIP}"
    echo "   Fallite (exit≠0)    : ${TOT_FAIL}"
    printf "   Tempo totale        : %dm %ds\n" $((ELAPSED/60)) $((ELAPSED%60))
    echo "============================================================"
} | tee -a "$OUTPUT"

echo "$(date '+%H:%M:%S') Fine — OK=${TOT_OK} SKIP=${TOT_SKIP} ERR=${TOT_FAIL} elapsed=${ELAPSED}s" >> "$LOG_FILE"
echo ""
echo "Risultati salvati in: ${OUTPUT}"

# Exit code non-zero se ci sono errori
[ "${TOT_FAIL}" -eq 0 ] || exit 1
