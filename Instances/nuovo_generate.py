new_code = '''#!/usr/bin/env python3
"""
Cross-Docking Instance Generator
Simulates Melián-Batista et al. (2023) instance format, following conventions
of Nassief et al. (2016) and Sayed (2020).

File naming: cd_n{N:04d}_m{M:04d}_d{D:02d}_{scenario}_d{density}_s{seed}.dzn

Truck sizes  : 8, 9, 10, 11, 12, 15, 20, 50  (same count for inbound and outbound)
Door counts  : 4, 5, 6, 7, 10, 30             (same count for inbound and outbound)

RNG streams use large seed offsets for stable reproducibility:
  rng_release  = Random(seed)           # arrival times
  rng_process  = Random(seed + 1000)    # unload / load times
  rng_transfer = Random(seed + 2000)    # pallet flow matrix
Adding/removing calls in one stream never affects the others.
"""

import random
import os

# ── Size / door configuration ─────────────────────────────────────────────────
TRUCK_SIZES = [8, 9, 10, 11, 12, 15, 20, 50]
DOOR_COUNTS = [4, 5, 6, 7, 10, 30]

# ── Seeds ─────────────────────────────────────────────────────────────────────
SEEDS = [1, 2, 3, 4, 5]


# ── DZN serialisation ─────────────────────────────────────────────────────────
def _dzn_array(values):
    return "[" + ",".join(str(v) for v in values) + "]"


def _dzn_matrix(matrix):
    rows = " | ".join(",".join(str(v) for v in row) for row in matrix)
    return "[| " + rows + " |]"


def write_dzn(path, n_in, n_out, d_in, d_out, release, unload, load, transfer):
    lines = [
        f"InboundTrucks  = {n_in};",
        f"OutboundTrucks = {n_out};",
        f"InboundDoors   = {d_in};",
        f"OutboundDoors  = {d_out};",
        f"ReleaseTime    = {_dzn_array(release)};",
        f"UnloadTime     = {_dzn_array(unload)};",
        f"LoadTime       = {_dzn_array(load)};",
        f"TransferTime   = {_dzn_matrix(transfer)};",
    ]
    with open(path, "w") as f:
        f.write("\\n".join(lines) + "\\n")


# ── Transfer matrix helper ────────────────────────────────────────────────────
def _transfer_matrix(n_in, n_out, rng, prob_fn, val_fn):
    """
    Build an n_in x n_out transfer matrix.
    prob_fn(i, j) -> float  : probability that cell (i,j) is non-zero
    val_fn()      -> int    : pallet count when non-zero
    Always draws exactly TWO values from rng per cell (one for probability,
    one for the value), regardless of the outcome, so the stream is stable.
    """
    matrix = []
    for i in range(n_in):
        row = []
        for j in range(n_out):
            p = rng.random()        # probability draw
            v = val_fn(rng)         # value draw (always consumed)
            row.append(v if p < prob_fn(i, j) else 0)
        matrix.append(row)
    return matrix


# ── Scenario generators ───────────────────────────────────────────────────────
def gen_uniform(n_in, n_out, seed):
    """Hub a regime – arrivi distribuiti [0,120], densita 75%."""
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload   = [rng_p.randint(10, 45) for _ in range(n_in)]
    load     = [rng_p.randint(5,  20) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.75,
                                val_fn =lambda r: r.randint(10, 50))
    return release, unload, load, transfer, 75


def gen_sparse(n_in, n_out, seed):
    """Hub nicchia – stesse release/process del base, matrice rada 25%."""
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload   = [rng_p.randint(10, 45) for _ in range(n_in)]
    load     = [rng_p.randint(5,  20) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.25,
                                val_fn =lambda r: r.randint(10, 50))
    return release, unload, load, transfer, 25


def gen_clustered(n_in, n_out, seed, n_clusters=3):
    """Hub multi-cliente – intra-cluster 80%, inter-cluster 8%, densita ~35%."""
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload  = [rng_p.randint(10, 45) for _ in range(n_in)]
    load    = [rng_p.randint(5,  20) for _ in range(n_out)]

    in_cl  = [i % n_clusters for i in range(n_in)]
    out_cl = [j % n_clusters for j in range(n_out)]

    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.80 if in_cl[i] == out_cl[j] else 0.08,
                                val_fn =lambda r: r.randint(10, 50))
    return release, unload, load, transfer, 35


def gen_asymmetric(n_in, n_out, seed):
    """Collo di bottiglia – scarico lento (20-45), carico veloce (5-15), densita 50%."""
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload   = [rng_p.randint(20, 45) for _ in range(n_in)]
    load     = [rng_p.randint(5,  15) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.50,
                                val_fn =lambda r: r.randint(10, 50))
    return release, unload, load, transfer, 50


def gen_congested(n_in, n_out, seed):
    """Ora di punta – tutti arrivano in [0,5], tempi alti, densita 75%."""
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 5)   for _ in range(n_in)]
    unload   = [rng_p.randint(20, 45) for _ in range(n_in)]
    load     = [rng_p.randint(15, 30) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.75,
                                val_fn =lambda r: r.randint(10, 50))
    return release, unload, load, transfer, 75


def gen_urgent(n_in, n_out, seed, urgent_frac=0.20):
    """
    Corriere espresso - 20% truck urgenti: arrivo [0,20], scarico [5,15].
    Restanti 80%: arrivo [20,120], scarico [10,45].
    Lato outbound: 20% truck urgenti con carico [5,15].
    Transfer: urgenti -> pochi destinatari (60% intra-urgent, 5% fuori);
              normali -> 35%.
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    n_urg_in  = max(1, round(n_in  * urgent_frac))
    n_urg_out = max(1, round(n_out * urgent_frac))

    release = [rng_r.randint(0,  20) if i < n_urg_in else rng_r.randint(20, 120)
               for i in range(n_in)]
    unload  = [rng_p.randint(5,  15) if i < n_urg_in else rng_p.randint(10,  45)
               for i in range(n_in)]
    load    = [rng_p.randint(5,  15) if j < n_urg_out else rng_p.randint(10, 30)
               for j in range(n_out)]

    def prob(i, j):
        if i < n_urg_in:
            return 0.60 if j < n_urg_out else 0.05
        return 0.35

    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=prob,
                                val_fn =lambda r: r.randint(10, 50))
    return release, unload, load, transfer, 35


def gen_mixed(n_in, n_out, seed, n_clusters=3):
    """
    Giornata imprevedibile - cluster + variabilita a 3 velocita + outlier (40-50).
    Intra-cluster 70%, inter 15%; densita ~50%.
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release = [rng_r.randint(0, 120) for _ in range(n_in)]

    _cats = ["fast", "medium", "slow"]
    unload = []
    for _ in range(n_in):
        c = rng_p.choice(_cats)
        unload.append(rng_p.randint(5, 15) if c == "fast"
                      else rng_p.randint(15, 30) if c == "medium"
                      else rng_p.randint(30, 45))
    load = []
    for _ in range(n_out):
        c = rng_p.choice(_cats)
        load.append(rng_p.randint(5, 15) if c == "fast"
                    else rng_p.randint(10, 20) if c == "medium"
                    else rng_p.randint(15, 30))

    in_cl  = [i % n_clusters for i in range(n_in)]
    out_cl = [j % n_clusters for j in range(n_out)]

    def prob(i, j):
        return 0.70 if in_cl[i] == out_cl[j] else 0.15

    def val_mixed(r):
        v_base = r.randint(10, 50)
        is_outlier = r.random() < 0.08
        return r.randint(40, 50) if is_outlier else v_base

    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=prob,
                                val_fn =val_mixed)
    return release, unload, load, transfer, 50


# ── Dispatch table ────────────────────────────────────────────────────────────
SCENARIOS = {
    "uniform":    gen_uniform,
    "sparse":     gen_sparse,
    "clustered":  gen_clustered,
    "asymmetric": gen_asymmetric,
    "congested":  gen_congested,
    "urgent":     gen_urgent,
    "mixed":      gen_mixed,
}


# ── Main generation loop ──────────────────────────────────────────────────────
def generate_all(output_dir="Instances"):
    os.makedirs(output_dir, exist_ok=True)
    count = 0
    for n in TRUCK_SIZES:
        for d in DOOR_COUNTS:
            for scenario_name, gen_fn in SCENARIOS.items():
                for seed in SEEDS:
                    release, unload, load, transfer, density = gen_fn(n, n, seed)
                    fname = (f"cd_n{n:04d}_m{n:04d}_d{d:02d}_{scenario_name}"
                             f"_d{density}_s{seed}.dzn")
                    write_dzn(os.path.join(output_dir, fname),
                              n, n, d, d,
                              release, unload, load, transfer)
                    count += 1
    print(f"Generated {count} instances in \\'{output_dir}/\\'")
    return count


if __name__ == "__main__":
    generate_all()
'''

import os
os.makedirs("/home/user/output", exist_ok=True)
with open("/home/user/output/generated.py", "w") as f:
    f.write(new_code)

# Verifica rapida: esegui e conta
import subprocess
result = subprocess.run(["python3", "/home/user/output/generated.py"], capture_output=True, text=True)
print(result.stdout)
print(result.stderr)
# 8 truck sizes × 6 door counts × 7 scenarios × 5 seeds = 1680
print(f"Atteso: {8 * 6 * 7 * 5} istanze")