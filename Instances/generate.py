import random
import sys
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
# Funzioni di supporto
# ---------------------------------------------------------------------------

def make_release_times(n, scenario):
    """
    ReleaseTime NON ordinato: i truck arrivano in ordine casuale.
    Ordinare artificialmente favorirebbe ERT — lasciamo non ordinato
    perché l'algoritmo deve trovare lui l'ordine corretto.
    La finestra scala con n per mantenere discriminazione tra truck
    anche su istanze grandi (Boysen et al. [1]).
    """
    rt_max = max(20, n * 3)

    if scenario == "congested":
        # Tutti arrivano in una finestra stretta: congestione reale
        rt_max_cong = max(5, n // 8)
        return [random.randint(0, rt_max_cong) for _ in range(n)]

    elif scenario == "urgent":
        # 20% truck urgenti: arrivano tardi (alto RT) ma devono partire subito
        # Simula spedizioni express con finestra di partenza rigida [4]
        times = []
        for i in range(n):
            if i < max(1, n // 5):   # primo 20% = urgenti
                times.append(random.randint(rt_max // 2, rt_max))
            else:
                times.append(random.randint(0, rt_max // 3))
        random.shuffle(times)   # mescola: l'algoritmo non sa chi è urgente
        return times

    else:
        return [random.randint(0, rt_max) for _ in range(n)]


def make_processing_times(n, m, scenario):
    """
    UnloadTime (inbound) e LoadTime (outbound).
    Per scenari asimmetrici usa range più ampi: questo aumenta la varianza
    nei processing times, che è la condizione in cui NEH dà maggiori
    vantaggi rispetto a SPT puro (Nawaz et al. [3]).
    """
    if scenario == "asymmetric":
        # Range ampio: truck grandi (lenti) coesistono con truck piccoli (veloci)
        unload_time = [random.randint(1, 30) for _ in range(n)]
        load_time   = [random.randint(1, 20) for _ in range(m)]

    elif scenario == "congested":
        # Tempi medi-alti: la congestione è aggravata da scarichi lenti
        unload_time = [random.randint(5, 15) for _ in range(n)]
        load_time   = [random.randint(5, 15) for _ in range(m)]

    elif scenario == "urgent":
        # Truck urgenti (primo 20%): LoadTime basso [4]
        unload_time = []
        load_time_list = []
        for i in range(n):
            if i < max(1, n // 5):
                unload_time.append(random.randint(1, 4))    # scarico rapido
            else:
                unload_time.append(random.randint(3, 12))
        for j in range(m):
            if j < max(1, m // 5):
                load_time_list.append(random.randint(1, 3)) # carico rapido
            else:
                load_time_list.append(random.randint(3, 12))
        random.shuffle(unload_time)
        random.shuffle(load_time_list)
        return unload_time, load_time_list

    elif scenario == "mixed":
        # Eterogeneo: mescola tutte le classi di truck
        unload_time = []
        for i in range(n):
            cls = random.choice(["small", "medium", "large"])
            if cls == "small":
                unload_time.append(random.randint(1, 5))
            elif cls == "medium":
                unload_time.append(random.randint(4, 12))
            else:
                unload_time.append(random.randint(10, 25))
        load_time = []
        for j in range(m):
            cls = random.choice(["small", "medium", "large"])
            if cls == "small":
                load_time.append(random.randint(1, 5))
            elif cls == "medium":
                load_time.append(random.randint(4, 12))
            else:
                load_time.append(random.randint(10, 20))

    else:
        # uniform, sparse, clustered: distribuzione standard
        unload_time = [random.randint(1, 10) for _ in range(n)]
        load_time   = [random.randint(1, 10) for _ in range(m)]

    return unload_time, load_time


def make_transfer_matrix(n, m, scenario):
    """
    Matrice TransferTime [n × m].

    - uniform   : tutti i valori 1-5 (matrice densa, baseline)
    - sparse    : sparsità casuale 25% — ogni cella ha 75% prob di essere 0
    - clustered : struttura a BLOCCHI — famiglie di prodotti [1]
                  Divide inbound e outbound in K cluster.
                  Un inbound truck trasferisce merci quasi solo agli outbound
                  del suo cluster → simula rotte/destinazioni reali.
    - asymmetric: valori in range più ampio (1-10) per maggiore eterogeneità
    - congested : matrice densa, valori 2-8 (transfer time più lunghi)
    - urgent    : matrice sparsa (30%), urgenti hanno transfer 1-2
    - mixed     : blocchi + sparsità + alcuni valori alti
    """

    if scenario == "uniform":
        return [[random.randint(1, 5) for _ in range(m)] for _ in range(n)]

    elif scenario == "sparse":
        matrix = []
        for i in range(n):
            row = []
            for j in range(m):
                row.append(random.randint(1, 5) if random.random() < 0.25 else 0)
            matrix.append(row)
        return matrix

    elif scenario == "clustered":
        # Struttura a blocchi: K = sqrt(min(n,m)) cluster
        # Letteratura cross-docking usa 3-6 famiglie di prodotti [1]
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
                    # Stesso cluster: trasferimento garantito, tempo 1-5
                    row.append(random.randint(1, 5))
                else:
                    # Cluster diverso: 10% prob di trasferimento cross-cluster
                    row.append(random.randint(1, 3) if random.random() < 0.10 else 0)
            matrix.append(row)
        return matrix

    elif scenario == "asymmetric":
        # Range ampio 1-10, matrice densa
        return [[random.randint(1, 10) for _ in range(m)] for _ in range(n)]

    elif scenario == "congested":
        # Densa, valori medi-alti
        return [[random.randint(2, 8) for _ in range(m)] for _ in range(n)]

    elif scenario == "urgent":
        # Sparsa (30%), truck urgenti (primi n//5) hanno transfer basso
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
        return matrix

    elif scenario == "mixed":
        # Combinazione: blocchi + sparsità variabile + outlier ad alto costo
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
                    # 5% outlier ad alto costo di trasferimento
                    if random.random() < 0.05:
                        val = random.randint(8, 15)
                    row.append(val)
                else:
                    row.append(0)
            matrix.append(row)
        return matrix

    return [[random.randint(1, 5) for _ in range(m)] for _ in range(n)]


# ---------------------------------------------------------------------------
# Generazione vera e propria
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

# Nome file descrittivo: scenario e seed immediatamente visibili
filename = f"cd_n{n:04d}_m{m:04d}_{scenario}_s{seed}.dzn"
path = Path(__file__).parent / filename

with open(path, "w") as f:
    f.write(f"InboundTrucks  = {n};\n")
    f.write(f"OutboundTrucks = {m};\n")
    f.write(f"ReleaseTime    = {fmt_array(release_time)};\n")
    f.write(f"UnloadTime     = {fmt_array(unload_time)};\n")
    f.write(f"LoadTime       = {fmt_array(load_time)};\n")
    f.write(f"TransferTime   = [| {rows} |];\n")

print(f"Generated {path}")