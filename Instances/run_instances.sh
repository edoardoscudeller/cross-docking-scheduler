#!/opt/homebrew/bin/bash
# =============================================================================
# run_instances.sh — Generazione completa delle istanze Cross-Docking Scheduler
# =============================================================================
# Uso:
#   cd Instances/
#   bash run_instances.sh              # genera tutto (350 istanze)
#   bash run_instances.sh uniform      # genera solo scenario uniform
#   bash run_instances.sh sparse 42    # genera solo sparse con seed 42
#
# Prerequisiti:
#   - Python 3.10+ disponibile come python3
#   - generate.py nella stessa cartella di questo script
#
# Output:
#   - file .dzn generati nella cartella corrente (Instances/)
#   - log riepilogativo in generation.log
#
# Struttura istanze (v0.1 — allineata a Nassief 2016 / Mendeley benchmark):
#   10 classi di taglia × 7 scenari × 5 seed = 350 istanze totali
#   Unità temporale: 1 u.t. = 1 minuto
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GENERATOR="${SCRIPT_DIR}/generate.py"
LOG_FILE="${SCRIPT_DIR}/generation.log"

# ---------------------------------------------------------------------------
# Filtri opzionali da argomento CLI
# ---------------------------------------------------------------------------
FILTER_SCENARIO="${1:-}"
FILTER_SEED="${2:-}"

# ---------------------------------------------------------------------------
# Classi di taglia: (n, m, d_in, d_out)
# Porte benchmark da Nassief 2016 / DOOR_CONFIGS in generate.py
# ---------------------------------------------------------------------------
declare -a N_LIST=(8   12   20   40   60   80   100  120  200  300)
declare -a M_LIST=(5   8    13   20   30   40   50   60   100  150)
declare -a DIN_LIST=(4  4    5    6    6    7    10   10   20   20)
declare -a DOUT_LIST=(4 4    5    6    6    7    10   10   20   20)

# ---------------------------------------------------------------------------
# Scenari e densità target corrispondenti (Nassief 2016)
# Funzione case invece di declare -A per compatibilità bash 3.x/macOS
# ---------------------------------------------------------------------------
density_for_scenario() {
    case "$1" in
        uniform)    echo "0.75" ;;
        sparse)     echo "0.25" ;;
        clustered)  echo "0.35" ;;
        asymmetric) echo "0.50" ;;
        congested)  echo "0.75" ;;
        urgent)     echo "0.35" ;;
        mixed)      echo "0.50" ;;
        *)          echo "0.50" ;;
    esac
}

SCENARIOS=(uniform sparse clustered asymmetric congested urgent mixed)

# ---------------------------------------------------------------------------
# Seed standardizzati: 5 repliche per robustezza statistica
# ---------------------------------------------------------------------------
SEEDS=(42 101 202 314 999)

# ---------------------------------------------------------------------------
# Contatori e log
# ---------------------------------------------------------------------------
COUNT_OK=0
COUNT_WARN=0
COUNT_ERR=0
START_TIME=$(date +%s)

echo "======================================================" | tee "${LOG_FILE}"
echo " Cross-Docking Scheduler — Generazione Istanze v0.1" | tee -a "${LOG_FILE}"
echo " Data: $(date '+%Y-%m-%d %H:%M:%S')" | tee -a "${LOG_FILE}"
echo " Generatore: ${GENERATOR}" | tee -a "${LOG_FILE}"
[ -n "${FILTER_SCENARIO}" ] && echo " Filtro scenario: ${FILTER_SCENARIO}" | tee -a "${LOG_FILE}"
[ -n "${FILTER_SEED}" ]     && echo " Filtro seed:     ${FILTER_SEED}"     | tee -a "${LOG_FILE}"
echo "======================================================" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# ---------------------------------------------------------------------------
# Funzione di generazione singola istanza
# ---------------------------------------------------------------------------
generate_instance() {
    local n=$1 m=$2 d_in=$3 d_out=$4 scenario=$5 seed=$6
    local density
    density="$(density_for_scenario "${scenario}")"

    local output
    output=$(python3 "${GENERATOR}" \
        "${n}" "${m}" "${seed}" "${scenario}" \
        --d_in "${d_in}" --d_out "${d_out}" \
        --density "${density}" 2>&1)

    local exit_code=$?

    if echo "${output}" | grep -q "\[WARN\]"; then
        echo "  [WARN] n=${n} m=${m} sc=${scenario} s=${seed}: ${output}" | tee -a "${LOG_FILE}"
        COUNT_WARN=$((COUNT_WARN + 1))
    elif [ "${exit_code}" -ne 0 ]; then
        echo "  [ERR]  n=${n} m=${m} sc=${scenario} s=${seed}: ${output}" | tee -a "${LOG_FILE}"
        COUNT_ERR=$((COUNT_ERR + 1))
    else
        echo "  [OK]   ${output}" | tee -a "${LOG_FILE}"
        COUNT_OK=$((COUNT_OK + 1))
    fi
}

# ---------------------------------------------------------------------------
# Loop principale
# ---------------------------------------------------------------------------
NUM_SIZES=${#N_LIST[@]}

for (( idx=0; idx<NUM_SIZES; idx++ )); do
    n="${N_LIST[$idx]}"
    m="${M_LIST[$idx]}"
    d_in="${DIN_LIST[$idx]}"
    d_out="${DOUT_LIST[$idx]}"

    echo "--- Taglia n=${n} m=${m} | porte in=${d_in} out=${d_out} ---" | tee -a "${LOG_FILE}"

    for scenario in "${SCENARIOS[@]}"; do
        if [ -n "${FILTER_SCENARIO}" ] && [ "${scenario}" != "${FILTER_SCENARIO}" ]; then
            continue
        fi

        for seed in "${SEEDS[@]}"; do
            if [ -n "${FILTER_SEED}" ] && [ "${seed}" != "${FILTER_SEED}" ]; then
                continue
            fi

            generate_instance "${n}" "${m}" "${d_in}" "${d_out}" "${scenario}" "${seed}"
        done
    done
done

# ---------------------------------------------------------------------------
# Riepilogo finale
# ---------------------------------------------------------------------------
END_TIME=$(date +%s)
ELAPSED=$(( END_TIME - START_TIME ))

echo "" | tee -a "${LOG_FILE}"
echo "======================================================" | tee -a "${LOG_FILE}"
echo " Riepilogo generazione" | tee -a "${LOG_FILE}"
echo "   Istanze OK    : ${COUNT_OK}" | tee -a "${LOG_FILE}"
echo "   Avvisi (WARN) : ${COUNT_WARN}" | tee -a "${LOG_FILE}"
echo "   Errori (ERR)  : ${COUNT_ERR}" | tee -a "${LOG_FILE}"
printf  "   Tempo totale  : %dm %ds\n" $((ELAPSED/60)) $((ELAPSED%60)) | tee -a "${LOG_FILE}"
echo "   Log salvato in: ${LOG_FILE}" | tee -a "${LOG_FILE}"
echo "======================================================" | tee -a "${LOG_FILE}"

if [ "${COUNT_ERR}" -gt 0 ]; then
    echo ""
    echo "[ATTENZIONE] ${COUNT_ERR} istanze non generate. Controlla ${LOG_FILE}."
    exit 1
fi
