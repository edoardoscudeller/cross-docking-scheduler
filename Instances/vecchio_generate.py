#!/usr/bin/env python3
"""
generate.py — Generatore di istanze per Cross-Docking Scheduler
================================================================
Uso:
    python3 generate.py <n> <m> <seed> <scenario> [--d_in D] [--d_out D] [--density DENSITY]

Argomenti posizionali:
    n           Numero di truck inbound
    m           Numero di truck outbound
    seed        Seed per la riproducibilità (intero)
    scenario    Tipo di istanza: uniform | sparse | clustered | asymmetric |
                                 congested | urgent | mixed

Argomenti opzionali:
    --d_in  D       Numero di porte inbound  (default: auto da DOOR_CONFIGS)
    --d_out D       Numero di porte outbound (default: auto da DOOR_CONFIGS)
    --density D     Densità target della matrice transfer [0.0-1.0]
                    Se non specificata, usa il valore di default dello scenario

Unità temporale: 1 u.t. = 1 minuto
    - release_time : U[0, 120]      arrivi nell'arco di 2 ore
    - unload_time  : U[10, 45]      minuti per scaricare un truck inbound
    - load_time    : U[5, 30]       minuti per caricare un truck outbound
    - transfer     : U[10, 50]      pallet da spostare (Nassief 2016)

Densità supportate: 0.25, 0.35, 0.50, 0.75 (come benchmark Nassief 2016 / Mendeley)

Esempi:
    python3 generate.py 8 5 42 uniform
    python3 generate.py 8 5 42 uniform --d_in 4 --d_out 4
    python3 generate.py 20 15 101 sparse --density 0.25
    python3 generate.py 50 50 314 clustered --d_in 6 --d_out 6 --density 0.50
"""

import random
import sys
import math
import argparse
from pathlib import Path

# ---------------------------------------------------------------------------
# Configurazioni porte benchmark (Nassief 2016 / Mendeley dataset)
# I valori sono indipendenti da n e m — NON derivati da formule ceil(n/k).
# Fonte: combinazioni da Sayed (2020) e Nassief et al. (2016).
# ---------------------------------------------------------------------------
DOOR_CONFIGS = {
    # (n_max, m_max) : (d_in, d_out)
    (12,  12):  (4,  4),
    (20,  20):  (5,  5),
    (30,  30):  (6,  6),
    (50,  50):  (7,  7),
    (100, 100): (12, 12),
    (200, 200): (25, 25),
    (500, 500): (50, 50),
}

# Seed standardizzati: 5 repliche per istanza (robusti per analisi statistica)
SEEDS_PER_INSTANCE = [42, 101, 202, 314, 999]

# Densità target supportate (Nassief 2016)
VALID_DENSITIES = {0.25, 0.35, 0.50, 0.75}

# Default densità per scenario
SCENARIO_DEFAULT_DENSITY = {
    "uniform":    0.75,
    "sparse":     0.25,
    "clustered":  0.35,
    "asymmetric": 0.50,
    "congested":  0.75,
    "urgent":     0.35,
    "mixed":      0.50,
}


# ---------------------------------------------------------------------------
# Selezione automatica porte da tabella benchmark
# ---------------------------------------------------------------------------
def select_door_config(n: int, m: int) -> tuple[int, int]:
    """
    Seleziona d_in e d_out dalla tabella DOOR_CONFIGS in base alla taglia
    dell'istanza. Usa la prima soglia >= max(n, m).
    Se n o m superano tutti i bound configurati, usa il massimo disponibile.
    """
    bound = max(n, m)
    for (n_max, m_max), (d_in, d_out) in sorted(DOOR_CONFIGS.items()):
        if bound <= n_max:
            return d_in, d_out
    # Fallback per istanze molto grandi
    return 30, 30


# ---------------------------------------------------------------------------
# enforce_density: garantisce che la matrice rispetti la densità target ±2%
# ---------------------------------------------------------------------------
def enforce_density(
    matrix: list[list[int]],
    n: int,
    m: int,
    target_density: float,
    rng: random.Random,
) -> list[list[int]]:
    """
    Aggiusta la matrice transfer per raggiungere target_density ± 2%.

    Vincoli rispettati dopo l'aggiustamento:
      - ogni riga ha almeno 1 valore nonzero (ogni truck in spedisce a >= 1 out)
      - ogni colonna ha almeno 1 valore nonzero (ogni truck out riceve da >= 1 in)
      - i nuovi valori inseriti sono in U[10, 50] (Nassief 2016)

    Args:
        matrix        : matrice n×m da aggiustare (modificata in-place e restituita)
        n, m          : dimensioni
        target_density: densità obiettivo (frazione di celle nonzero)
        rng           : istanza Random per riproducibilità

    Returns:
        La matrice aggiustata.
    """
    total = n * m
    target_nonzero = round(target_density * total)

    def count_nonzero():
        return sum(1 for row in matrix for v in row if v > 0)

    current_nz = count_nonzero()

    # ---- fase 1: aggiungi celle se necessario ----
    if current_nz < target_nonzero:
        zero_cells = [(i, j) for i in range(n) for j in range(m) if matrix[i][j] == 0]
        rng.shuffle(zero_cells)
        for i, j in zero_cells:
            if current_nz >= target_nonzero:
                break
            matrix[i][j] = rng.randint(10, 50)
            current_nz += 1

    # ---- fase 2: rimuovi celle se in eccesso ----
    elif current_nz > target_nonzero:
        nonzero_cells = [(i, j) for i in range(n) for j in range(m) if matrix[i][j] > 0]
        rng.shuffle(nonzero_cells)
        for i, j in nonzero_cells:
            if current_nz <= target_nonzero:
                break
            # Non rimuovere se è l'unico nonzero nella riga o nella colonna
            row_nz = sum(1 for v in matrix[i] if v > 0)
            col_nz = sum(1 for ii in range(n) if matrix[ii][j] > 0)
            if row_nz > 1 and col_nz > 1:
                matrix[i][j] = 0
                current_nz -= 1

    # ---- fase 3: verifica e ripristino vincoli di copertura ----
    for i in range(n):
        if all(v == 0 for v in matrix[i]):
            j = rng.randint(0, m - 1)
            matrix[i][j] = rng.randint(10, 50)

    for j in range(m):
        if all(matrix[i][j] == 0 for i in range(n)):
            i = rng.randint(0, n - 1)
            matrix[i][j] = rng.randint(10, 50)

    return matrix


# ---------------------------------------------------------------------------
# Release time
# ---------------------------------------------------------------------------
def make_release_times(n: int, scenario: str, rng: random.Random) -> list[int]:
    """
    Genera i release time dei truck inbound.
    1 u.t. = 1 minuto; orizzonte base = 120 minuti (2 ore).

    Scenari:
        uniform/sparse/clustered/asymmetric/mixed : U[0, 120]
        congested  : U[0, max(5, n//4)] — arrivi concentrati (finestra stretta)
        urgent     : ~20% truck arrivano subito U[0,20], resto U[0,60]
    """
    T_MAX = 120  # 2 ore in minuti

    if scenario == "congested":
        rt_max = max(5, n // 4)  # finestra stretta ma con variabilità minima
        return [rng.randint(0, rt_max) for _ in range(n)]

    elif scenario == "urgent":
        n_urgent = max(1, n // 5)
        times = []
        for i in range(n):
            if i < n_urgent:
                times.append(rng.randint(0, 20))   # SLA critici: arrivano subito
            else:
                times.append(rng.randint(0, 60))   # resto: prima ora
        rng.shuffle(times)
        return times

    else:
        return [rng.randint(0, T_MAX) for _ in range(n)]


# ---------------------------------------------------------------------------
# Processing time (unload + load)
# ---------------------------------------------------------------------------
def make_processing_times(n: int, m: int, scenario: str, rng: random.Random) -> tuple[list[int], list[int]]:
    """
    Genera unload_time (per truck inbound) e load_time (per truck outbound).
    1 u.t. = 1 minuto.

    Range di riferimento (Nassief-compatibili):
        unload_time : U[10, 45] — default
        load_time   : U[5, 30]  — default

    Variazioni per scenario:
        asymmetric : unload molto alto (collo di bottiglia inbound)
        congested  : tempi alti per entrambi (traffico intenso alle porte)
        urgent     : ~20% truck con tempi molto bassi (priorità SLA)
        mixed      : mix casuale small/medium/large
    """
    if scenario == "asymmetric":
        # Collo di bottiglia lato inbound: scarico lento, carico veloce
        unload_time = [rng.randint(20, 45) for _ in range(n)]
        load_time   = [rng.randint(5,  15) for _ in range(m)]

    elif scenario == "congested":
        # Alta occupazione delle porte: tempi di processo elevati
        unload_time = [rng.randint(20, 45) for _ in range(n)]
        load_time   = [rng.randint(15, 30) for _ in range(m)]

    elif scenario == "urgent":
        n_urgent = max(1, n // 5)
        m_urgent = max(1, m // 5)
        unload_list = []
        for i in range(n):
            if i < n_urgent:
                unload_list.append(rng.randint(5, 15))  # truck urgenti: scarico rapido
            else:
                unload_list.append(rng.randint(10, 45))
        rng.shuffle(unload_list)

        load_list = []
        for j in range(m):
            if j < m_urgent:
                load_list.append(rng.randint(3, 10))    # truck urgenti: carico rapido
            else:
                load_list.append(rng.randint(5, 30))
        rng.shuffle(load_list)
        return unload_list, load_list

    elif scenario == "mixed":
        def mixed_times(count, ranges):
            result = []
            for _ in range(count):
                cls = rng.choice(["small", "medium", "large"])
                result.append(rng.randint(*ranges[cls]))
            return result

        unload_ranges = {"small": (10, 15), "medium": (15, 30), "large": (30, 45)}
        load_ranges   = {"small": (5,  10), "medium": (10, 20), "large": (20, 30)}
        unload_time = mixed_times(n, unload_ranges)
        load_time   = mixed_times(m, load_ranges)

    else:
        # uniform, sparse, clustered, urgent (default)
        unload_time = [rng.randint(10, 45) for _ in range(n)]
        load_time   = [rng.randint(5,  30) for _ in range(m)]

    return unload_time, load_time


# ---------------------------------------------------------------------------
# Transfer matrix
# ---------------------------------------------------------------------------
def make_transfer_matrix(n: int, m: int, scenario: str, target_density: float, rng: random.Random) -> list[list[int]]:
    """
    Genera la matrice transfer n×m con valori in U[10, 50] (Nassief 2016).
    La densità effettiva viene portata a target_density ± 2% da enforce_density.

    Struttura per scenario:
        uniform    : tutte le celle nonzero
        sparse     : ~target_density di celle nonzero distribuite casualmente
        clustered  : blocchi diagonali (fornitori dedicati a clienti)
        asymmetric : fully dense (collo di bottiglia nei tempi, non nel flusso)
        congested  : fully dense (alta congestione alle porte)
        urgent     : ~20% truck urgenti con flusso concentrato e limitato
        mixed      : struttura a cluster con rumore casuale
    """
    # ---- uniform ----
    if scenario == "uniform":
        matrix = [[rng.randint(10, 50) for _ in range(m)] for _ in range(n)]
        return enforce_density(matrix, n, m, target_density, rng)

    # ---- sparse ----
    elif scenario == "sparse":
        matrix = []
        for i in range(n):
            row = [rng.randint(10, 50) if rng.random() < target_density else 0
                   for _ in range(m)]
            matrix.append(row)
        return enforce_density(matrix, n, m, target_density, rng)

    # ---- clustered ----
    elif scenario == "clustered":
        # K = numero di cluster; per n<=12 forza K=2 per evitare degenerazione
        K = max(2, min(4, int(math.sqrt(min(n, m)))))
        in_cluster  = [i % K for i in range(n)]
        out_cluster = [j % K for j in range(m)]
        rng.shuffle(in_cluster)
        rng.shuffle(out_cluster)

        matrix = []
        for i in range(n):
            row = []
            for j in range(m):
                if in_cluster[i] == out_cluster[j]:
                    row.append(rng.randint(10, 50))   # flusso nel cluster
                else:
                    # piccola probabilità di flusso inter-cluster (rumore realistico)
                    row.append(rng.randint(10, 50) if rng.random() < 0.08 else 0)
            matrix.append(row)
        return enforce_density(matrix, n, m, target_density, rng)

    # ---- asymmetric ----
    elif scenario == "asymmetric":
        matrix = [[rng.randint(10, 50) for _ in range(m)] for _ in range(n)]
        return enforce_density(matrix, n, m, target_density, rng)

    # ---- congested ----
    elif scenario == "congested":
        # Alta densità: quasi tutte le porte ricevono flusso
        matrix = [[rng.randint(10, 50) for _ in range(m)] for _ in range(n)]
        return enforce_density(matrix, n, m, target_density, rng)

    # ---- urgent ----
    elif scenario == "urgent":
        n_urgent = max(1, n // 5)
        matrix = []
        for i in range(n):
            row = []
            for j in range(m):
                if i < n_urgent:
                    # truck urgenti: pochi pallet, concentrati su poche destinazioni
                    row.append(rng.randint(10, 30) if rng.random() < 0.40 else 0)
                else:
                    row.append(rng.randint(10, 50) if rng.random() < target_density else 0)
            matrix.append(row)
        return enforce_density(matrix, n, m, target_density, rng)

    # ---- mixed ----
    elif scenario == "mixed":
        K = max(2, min(4, int(math.sqrt(min(n, m)))))
        in_cluster  = [i % K for i in range(n)]
        out_cluster = [j % K for j in range(m)]
        rng.shuffle(in_cluster)
        rng.shuffle(out_cluster)

        matrix = []
        for i in range(n):
            row = []
            for j in range(m):
                same = (in_cluster[i] == out_cluster[j])
                prob = 0.60 if same else 0.15
                if rng.random() < prob:
                    # occasionalmente un carico molto pesante (outlier realistico)
                    if rng.random() < 0.05:
                        row.append(rng.randint(40, 50))
                    else:
                        row.append(rng.randint(10, 50))
                else:
                    row.append(0)
            matrix.append(row)
        return enforce_density(matrix, n, m, target_density, rng)

    # fallback
    matrix = [[rng.randint(10, 50) for _ in range(m)] for _ in range(n)]
    return enforce_density(matrix, n, m, target_density, rng)


# ---------------------------------------------------------------------------
# Parsing argomenti CLI
# ---------------------------------------------------------------------------
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generatore di istanze .dzn per Cross-Docking Scheduler",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("n",        type=int, help="Numero di truck inbound")
    parser.add_argument("m",        type=int, help="Numero di truck outbound")
    parser.add_argument("seed",     type=int, help="Seed RNG")
    parser.add_argument(
        "scenario",
        choices=["uniform", "sparse", "clustered", "asymmetric", "congested", "urgent", "mixed"],
        help="Tipo di istanza",
    )
    parser.add_argument(
        "--d_in",
        type=int,
        default=None,
        help="Numero porte inbound (default: auto da tabella benchmark)",
    )
    parser.add_argument(
        "--d_out",
        type=int,
        default=None,
        help="Numero porte outbound (default: auto da tabella benchmark)",
    )
    parser.add_argument(
        "--density",
        type=float,
        default=None,
        help=f"Densità target matrice transfer {{0.25, 0.35, 0.50, 0.75}} "
             f"(default: per-scenario)",
    )
    return parser.parse_args()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> None:
    args = parse_args()
    n        = args.n
    m        = args.m
    seed     = args.seed
    scenario = args.scenario

    # Validazione densità
    if args.density is not None:
        if args.density not in VALID_DENSITIES:
            print(
                f"[WARN] --density {args.density} non è una densità standard "
                f"{sorted(VALID_DENSITIES)}. Verrà usata comunque."
            )
        target_density = args.density
    else:
        target_density = SCENARIO_DEFAULT_DENSITY[scenario]

    # Selezione porte
    d_in_auto, d_out_auto = select_door_config(n, m)
    d_in  = args.d_in  if args.d_in  is not None else d_in_auto
    d_out = args.d_out if args.d_out is not None else d_out_auto

    # Avvisi se le porte sono fisicamente inconsistenti
    if d_in > n:
        print(f"[WARN] d_in={d_in} > n={n}: porte inbound superiori ai truck. "
              f"L'istanza è valida ma degenere.")
    if d_out > m:
        print(f"[WARN] d_out={d_out} > m={m}: porte outbound superiori ai truck. "
              f"L'istanza è valida ma degenere.")

    # RNG dedicato: garantisce riproducibilità indipendente dall'ordine di generazione
    rng = random.Random(seed)

    # Generazione componenti
    release_time          = make_release_times(n, scenario, rng)
    unload_time, load_time = make_processing_times(n, m, scenario, rng)
    transfer_time         = make_transfer_matrix(n, m, scenario, target_density, rng)

    # Calcolo densità effettiva (per verifica e log)
    actual_nz      = sum(1 for row in transfer_time for v in row if v > 0)
    actual_density = actual_nz / (n * m)

    # ---------------------------------------------------------------------------
    # Scrittura file .dzn
    # ---------------------------------------------------------------------------
    def fmt_array(v: list) -> str:
        return "[" + ",".join(map(str, v)) + "]"

    def fmt_matrix(mat: list[list[int]]) -> str:
        rows = " | ".join(",".join(map(str, row)) for row in mat)
        return f"[| {rows} |]"

    density_tag  = f"d{int(target_density * 100):02d}"
    filename     = f"cd_n{n:04d}_m{m:04d}_{scenario}_{density_tag}_s{seed}.dzn"
    output_path  = Path(__file__).parent / filename

    with open(output_path, "w") as f:
        f.write(f"InboundTrucks  = {n};\n")
        f.write(f"OutboundTrucks = {m};\n")
        f.write(f"InboundDoors   = {d_in};\n")
        f.write(f"OutboundDoors  = {d_out};\n")
        f.write(f"ReleaseTime    = {fmt_array(release_time)};\n")
        f.write(f"UnloadTime     = {fmt_array(unload_time)};\n")
        f.write(f"LoadTime       = {fmt_array(load_time)};\n")
        f.write(f"TransferTime   = {fmt_matrix(transfer_time)};\n")

    print(
        f"[OK] {filename}\n"
        f"     trucks: {n} in / {m} out  |  porte: {d_in} in / {d_out} out\n"
        f"     densità: target={target_density:.0%}  effettiva={actual_density:.1%}\n"
        f"     seed={seed}  scenario={scenario}"
    )


if __name__ == "__main__":
    main()
