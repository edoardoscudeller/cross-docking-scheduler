import random
import sys
import math
from pathlib import Path



# ---------------------------------------------------------------------------
# Parsing argomenti
# ---------------------------------------------------------------------------
if len(sys.argv) < 3:
    print("Uso: python3 generate.py <n_inbound> <n_outbound> [seed] [scenario]")
    print("Scenari: uniform (default), sparse, asymmetric, congested, urgent, clustered, mixed")
    sys.exit(1)

n        = int(sys.argv[1])
m        = int(sys.argv[2])
seed     = int(sys.argv[3]) if len(sys.argv) > 3 else n * m + 7
scenario = sys.argv[4].lower() if len(sys.argv) > 4 else "uniform"


if scenario not in {"uniform", "sparse", "asymmetric", "congested", "urgent", "clustered", "mixed"}:
    print(f"Scenario '{scenario}' non riconosciuto.")
    print("Scenari validi: uniform, sparse, asymmetric, congested, urgent, clustered, mixed")
    sys.exit(1)


random.seed(seed)



# ---------------------------------------------------------------------------
# Calcolo porte in base alla dimensione dell'istanza
#
# Rapporto d_in : d_out = 2:1, coerente con il rapporto n ≈ 2*m
# adottato nella letteratura benchmark (Boysen et al. 2010, Miao et al. 2009).
#
# Regola:
#   n <= 20        : d_in = 2,  d_out = 1  (minimo fisico plausibile)
#   20 < n <= 80  : d_in = ceil(n/5),  d_out = ceil(m/5)
#                    coerente con le configurazioni di Boysen et al. (2010) e Nassief et al. (2016)
#   n > 80        : d_in = max(16, ceil(n/8)),  d_out = max(16, ceil(m/8))
#                    il max() garantisce continuità al confine n=150.
# # ---------------------------------------------------------------------------
def compute_inbound_doors(n):
    if n <= 20:
        return 2
    elif n <= 80:
        return math.ceil(n / 5)
    else:
        return max(16, math.ceil(n / 8))


def compute_outbound_doors(n, m):
    if n <= 20:
        return 1
    elif n <= 80:
        return math.ceil(m / 5)
    else:
        return max(16, math.ceil(m / 8))


d_in  = compute_inbound_doors(n)
d_out = compute_outbound_doors(n, m)



d_in  = compute_inbound_doors(n)
d_out = compute_outbound_doors(n, m)



# ---------------------------------------------------------------------------
# Funzioni di supporto
# ---------------------------------------------------------------------------


def make_release_times(n, scenario):
    rt_max = max(20, n * 3)
    if scenario == "congested":
        rt_max_cong = max(5, n // 8)
        return [random.randint(0, rt_max_cong) for _ in range(n)]
    elif scenario == "urgent":
        times = []
        for i in range(n):
            if i < max(1, n // 5):
                times.append(random.randint(0, rt_max // 6))
            else:
                times.append(random.randint(0, rt_max // 3))
        random.shuffle(times)
        return times
    else:
        return [random.randint(0, rt_max) for _ in range(n)]



def make_processing_times(n, m, scenario):
    if scenario == "asymmetric":
        unload_time = [random.randint(1, 30) for _ in range(n)]
        load_time   = [random.randint(1, 20) for _ in range(m)]
    elif scenario == "congested":
        unload_time = [random.randint(5, 15) for _ in range(n)]
        load_time   = [random.randint(5, 15) for _ in range(m)]
    elif scenario == "urgent":
        unload_time = []
        load_time_list = []
        for i in range(n):
            if i < max(1, n // 5):
                unload_time.append(random.randint(1, 4))
            else:
                unload_time.append(random.randint(3, 12))
        for j in range(m):
            if j < max(1, m // 5):
                load_time_list.append(random.randint(1, 3))
            else:
                load_time_list.append(random.randint(3, 12))
        random.shuffle(unload_time)
        random.shuffle(load_time_list)
        return unload_time, load_time_list
    elif scenario == "mixed":
        unload_time = []
        for i in range(n):
            cls = random.choice(["small", "medium", "large"])
            if cls == "small":   unload_time.append(random.randint(1, 5))
            elif cls == "medium":unload_time.append(random.randint(4, 12))
            else:                unload_time.append(random.randint(10, 25))
        load_time = []
        for j in range(m):
            cls = random.choice(["small", "medium", "large"])
            if cls == "small":   load_time.append(random.randint(1, 5))
            elif cls == "medium":load_time.append(random.randint(4, 12))
            else:                load_time.append(random.randint(10, 20))
    else:
        unload_time = [random.randint(1, 10) for _ in range(n)]
        load_time   = [random.randint(1, 10) for _ in range(m)]
    return unload_time, load_time



def make_transfer_matrix(n, m, scenario):
    if scenario == "uniform":
        return [[random.randint(1, 5) for _ in range(m)] for _ in range(n)]
    elif scenario == "sparse":
        matrix = []
        for i in range(n):
            row = [random.randint(1, 5) if random.random() < 0.25 else 0 for _ in range(m)]
            matrix.append(row)
        for i in range(n):
            if all(v == 0 for v in matrix[i]):
                j = random.randint(0, m - 1)
                matrix[i][j] = random.randint(1, 5)
        for j in range(m):
            if all(matrix[i][j] == 0 for i in range(n)):
                i = random.randint(0, n - 1)
                matrix[i][j] = random.randint(1, 5)
        return matrix
    elif scenario == "clustered":
        K = max(3, min(6, int((min(n, m)) ** 0.5)))
        in_cluster  = [i % K for i in range(n)]
        out_cluster = [j % K for j in range(m)]
        random.shuffle(in_cluster)
        random.shuffle(out_cluster)
        matrix = []
        for i in range(n):
            row = []
            for j in range(m):
                if in_cluster[i] == out_cluster[j]:
                    row.append(random.randint(1, 5))
                else:
                    row.append(random.randint(1, 3) if random.random() < 0.10 else 0)
            matrix.append(row)
        for i in range(n):
            if all(v == 0 for v in matrix[i]):
                j = random.randint(0, m - 1)
                matrix[i][j] = random.randint(1, 5)
        for j in range(m):
            if all(matrix[i][j] == 0 for i in range(n)):
                i = random.randint(0, n - 1)
                matrix[i][j] = random.randint(1, 5)
        return matrix
    elif scenario == "asymmetric":
        return [[random.randint(1, 10) for _ in range(m)] for _ in range(n)]
    elif scenario == "congested":
        return [[random.randint(2, 8) for _ in range(m)] for _ in range(n)]
    elif scenario == "urgent":
        n_urgent = max(1, n // 5)
        matrix = []
        for i in range(n):
            row = []
            for j in range(m):
                if i < n_urgent:
                    row.append(random.randint(1, 2) if random.random() < 0.40 else 0)
                else:
                    row.append(random.randint(1, 5) if random.random() < 0.30 else 0)
            matrix.append(row)
        for i in range(n):
            if all(v == 0 for v in matrix[i]):
                j = random.randint(0, m - 1)
                matrix[i][j] = random.randint(1, 2)
        for j in range(m):
            if all(matrix[i][j] == 0 for i in range(n)):
                i = random.randint(0, n - 1)
                matrix[i][j] = random.randint(1, 2)
        return matrix
    elif scenario == "mixed":
        K = max(3, min(5, int((min(n, m)) ** 0.5)))
        in_cluster  = [i % K for i in range(n)]
        out_cluster = [j % K for j in range(m)]
        random.shuffle(in_cluster)
        random.shuffle(out_cluster)
        matrix = []
        for i in range(n):
            row = []
            for j in range(m):
                same = (in_cluster[i] == out_cluster[j])
                prob = 0.55 if same else 0.12
                if random.random() < prob:
                    val = random.randint(1, 5) if same else random.randint(1, 8)
                    if random.random() < 0.05:
                        val = random.randint(8, 15)
                    row.append(val)
                else:
                    row.append(0)
            matrix.append(row)
        for i in range(n):
            if all(v == 0 for v in matrix[i]):
                j = random.randint(0, m - 1)
                matrix[i][j] = random.randint(1, 5)
        for j in range(m):
            if all(matrix[i][j] == 0 for i in range(n)):
                i = random.randint(0, n - 1)
                matrix[i][j] = random.randint(1, 5)
        return matrix
    return [[random.randint(1, 5) for _ in range(m)] for _ in range(n)]



# ---------------------------------------------------------------------------
# Generazione
# ---------------------------------------------------------------------------
release_time            = make_release_times(n, scenario)
unload_time, load_time  = make_processing_times(n, m, scenario)
transfer_time           = make_transfer_matrix(n, m, scenario)



# ---------------------------------------------------------------------------
# Scrittura file .dzn
# ---------------------------------------------------------------------------
def fmt_array(v):
    return "[" + ",".join(map(str, v)) + "]"


rows = " | ".join(",".join(map(str, row)) for row in transfer_time)
filename = f"cd_n{n:04d}_m{m:04d}_{scenario}_s{seed}.dzn"
path = Path(__file__).parent / filename


with open(path, "w") as f:
    f.write(f"InboundTrucks  = {n};\n")
    f.write(f"OutboundTrucks = {m};\n")
    f.write(f"InboundDoors   = {d_in};\n")
    f.write(f"OutboundDoors  = {d_out};\n")
    f.write(f"ReleaseTime    = {fmt_array(release_time)};\n")
    f.write(f"UnloadTime     = {fmt_array(unload_time)};\n")
    f.write(f"LoadTime       = {fmt_array(load_time)};\n")
    f.write(f"TransferTime   = [| {rows} |];\n")


print(f"Generated {path}  [d_in={d_in}, d_out={d_out}]")