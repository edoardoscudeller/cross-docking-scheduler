# generate.py
# Uso: python3 generate.py <n_inbound> <n_outbound> [seed]

import random
import sys
from pathlib import Path

n = int(sys.argv[1])
m = int(sys.argv[2])
seed = int(sys.argv[3]) if len(sys.argv) > 3 else 42

random.seed(seed)

release_time  = [random.randint(0, 20) for _ in range(n)]
unload_time   = [random.randint(1, 10) for _ in range(n)]
load_time     = [random.randint(1, 10) for _ in range(m)]
transfer_time = [[random.randint(1, 5) for _ in range(m)] for _ in range(n)]

def fmt_array(v):
    return "[" + ",".join(map(str, v)) + "]"

rows = " | ".join(",".join(map(str, row)) for row in transfer_time)

filename = f"cd_n{n:03d}_m{m:03d}.dzn"
path = Path("Instances") / filename

with open(path, "w") as f:
    f.write(f"InboundTrucks  = {n};\n")
    f.write(f"OutboundTrucks = {m};\n")
    f.write(f"ReleaseTime    = {fmt_array(release_time)};\n")
    f.write(f"UnloadTime     = {fmt_array(unload_time)};\n")
    f.write(f"LoadTime       = {fmt_array(load_time)};\n")
    f.write(f"TransferTime   = [| {rows} |];\n")

print(f"Generated {path}")