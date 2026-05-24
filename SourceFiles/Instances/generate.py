# generate.py
# Uso: python3 generate.py <n_inbound> <n_outbound> <seed> > cd_large.dzn

import random
import sys

n  = int(sys.argv[1])   # camion inbound
m  = int(sys.argv[2])   # camion outbound
seed = int(sys.argv[3]) if len(sys.argv) > 3 else 42
random.seed(seed)

release_time  = [random.randint(0, 20)  for _ in range(n)]
unload_time   = [random.randint(1, 10)  for _ in range(n)]
load_time     = [random.randint(1, 10)  for _ in range(m)]
transfer_time = [[random.randint(1, 5)  for _ in range(m)] for _ in range(n)]

print(f"InboundTrucks  = {n};")
print(f"OutboundTrucks = {m};")
print(f"ReleaseTime    = {release_time};")
print(f"UnloadTime     = {unload_time};")
print(f"LoadTime       = {load_time};")

rows = " | ".join(",".join(str(transfer_time[i][j]) for j in range(m)) for i in range(n))
print(f"TransferTime   = [| {rows} |];")