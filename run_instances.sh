#!/bin/bash
# run_instances.sh
# Genera tutte le istanze del progetto cross-docking-scheduler v3
# Uso: bash run_instances.sh
# Le istanze verranno salvate nella cartella Instances/

cd Instances

# ========================================================================
# SCENARIO: UNIFORM
# BASELINE — matrice densa uniforme, riferimento per tutti i confronti
# ========================================================================

python3 generate.py 8    5    101 uniform
python3 generate.py 8    5    111 uniform
python3 generate.py 8    5    121 uniform
python3 generate.py 12   8    102 uniform
python3 generate.py 12   8    112 uniform
python3 generate.py 12   8    122 uniform
python3 generate.py 20   13   103 uniform
python3 generate.py 20   13   113 uniform
python3 generate.py 20   13   123 uniform
python3 generate.py 40   20   201 uniform
python3 generate.py 60   30   202 uniform
python3 generate.py 80   40   203 uniform
python3 generate.py 100  50   204 uniform
python3 generate.py 120  60   205 uniform
python3 generate.py 200  100  301 uniform
python3 generate.py 300  150  302 uniform
python3 generate.py 400  200  303 uniform
python3 generate.py 500  250  304 uniform
python3 generate.py 600  300  305 uniform
python3 generate.py 40   20   211 uniform
python3 generate.py 100  50   212 uniform
python3 generate.py 300  150  213 uniform
python3 generate.py 600  300  214 uniform

# ========================================================================
# SCENARIO: SPARSE
# SPARSE — 25% densità, merce differenziata per destinazione
# ========================================================================

python3 generate.py 8    5    101 sparse
python3 generate.py 8    5    111 sparse
python3 generate.py 8    5    121 sparse
python3 generate.py 12   8    102 sparse
python3 generate.py 12   8    112 sparse
python3 generate.py 12   8    122 sparse
python3 generate.py 20   13   103 sparse
python3 generate.py 20   13   113 sparse
python3 generate.py 20   13   123 sparse
python3 generate.py 40   20   201 sparse
python3 generate.py 60   30   202 sparse
python3 generate.py 80   40   203 sparse
python3 generate.py 100  50   204 sparse
python3 generate.py 120  60   205 sparse
python3 generate.py 200  100  301 sparse
python3 generate.py 300  150  302 sparse
python3 generate.py 400  200  303 sparse
python3 generate.py 500  250  304 sparse
python3 generate.py 600  300  305 sparse
python3 generate.py 40   20   211 sparse
python3 generate.py 100  50   212 sparse
python3 generate.py 300  150  213 sparse
python3 generate.py 600  300  214 sparse

# ========================================================================
# SCENARIO: CLUSTERED
# CLUSTERED — famiglie di prodotti a blocchi (più realistico)
# ========================================================================

python3 generate.py 8    5    101 clustered
python3 generate.py 8    5    111 clustered
python3 generate.py 8    5    121 clustered
python3 generate.py 12   8    102 clustered
python3 generate.py 12   8    112 clustered
python3 generate.py 12   8    122 clustered
python3 generate.py 20   13   103 clustered
python3 generate.py 20   13   113 clustered
python3 generate.py 20   13   123 clustered
python3 generate.py 40   20   201 clustered
python3 generate.py 60   30   202 clustered
python3 generate.py 80   40   203 clustered
python3 generate.py 100  50   204 clustered
python3 generate.py 120  60   205 clustered
python3 generate.py 200  100  301 clustered
python3 generate.py 300  150  302 clustered
python3 generate.py 400  200  303 clustered
python3 generate.py 500  250  304 clustered
python3 generate.py 600  300  305 clustered
python3 generate.py 40   20   211 clustered
python3 generate.py 100  50   212 clustered
python3 generate.py 300  150  213 clustered
python3 generate.py 600  300  214 clustered

# ========================================================================
# SCENARIO: ASYMMETRIC
# ASYMMETRIC — truck eterogenei, amplifica vantaggi NEH vs SPT
# ========================================================================

python3 generate.py 8    5    101 asymmetric
python3 generate.py 8    5    111 asymmetric
python3 generate.py 8    5    121 asymmetric
python3 generate.py 12   8    102 asymmetric
python3 generate.py 12   8    112 asymmetric
python3 generate.py 12   8    122 asymmetric
python3 generate.py 20   13   103 asymmetric
python3 generate.py 20   13   113 asymmetric
python3 generate.py 20   13   123 asymmetric
python3 generate.py 40   20   201 asymmetric
python3 generate.py 60   30   202 asymmetric
python3 generate.py 80   40   203 asymmetric
python3 generate.py 100  50   204 asymmetric
python3 generate.py 120  60   205 asymmetric
python3 generate.py 200  100  301 asymmetric
python3 generate.py 300  150  302 asymmetric
python3 generate.py 400  200  303 asymmetric
python3 generate.py 500  250  304 asymmetric
python3 generate.py 600  300  305 asymmetric
python3 generate.py 40   20   211 asymmetric
python3 generate.py 100  50   212 asymmetric
python3 generate.py 300  150  213 asymmetric
python3 generate.py 600  300  214 asymmetric

# ========================================================================
# SCENARIO: CONGESTED
# CONGESTED — picco operativo, tutti arrivano quasi insieme
# ========================================================================

python3 generate.py 8    5    101 congested
python3 generate.py 8    5    111 congested
python3 generate.py 8    5    121 congested
python3 generate.py 12   8    102 congested
python3 generate.py 12   8    112 congested
python3 generate.py 12   8    122 congested
python3 generate.py 20   13   103 congested
python3 generate.py 20   13   113 congested
python3 generate.py 20   13   123 congested
python3 generate.py 40   20   201 congested
python3 generate.py 60   30   202 congested
python3 generate.py 80   40   203 congested
python3 generate.py 100  50   204 congested
python3 generate.py 120  60   205 congested
python3 generate.py 200  100  301 congested
python3 generate.py 300  150  302 congested
python3 generate.py 400  200  303 congested
python3 generate.py 500  250  304 congested
python3 generate.py 600  300  305 congested
python3 generate.py 40   20   211 congested
python3 generate.py 100  50   212 congested
python3 generate.py 300  150  213 congested
python3 generate.py 600  300  214 congested

# ========================================================================
# SCENARIO: URGENT
# URGENT — 20% truck express (RT alto, tempi bassi), test WSPT
# ========================================================================

python3 generate.py 8    5    101 urgent
python3 generate.py 8    5    111 urgent
python3 generate.py 8    5    121 urgent
python3 generate.py 12   8    102 urgent
python3 generate.py 12   8    112 urgent
python3 generate.py 12   8    122 urgent
python3 generate.py 20   13   103 urgent
python3 generate.py 20   13   113 urgent
python3 generate.py 20   13   123 urgent
python3 generate.py 40   20   201 urgent
python3 generate.py 60   30   202 urgent
python3 generate.py 80   40   203 urgent
python3 generate.py 100  50   204 urgent
python3 generate.py 120  60   205 urgent
python3 generate.py 200  100  301 urgent
python3 generate.py 300  150  302 urgent
python3 generate.py 400  200  303 urgent
python3 generate.py 500  250  304 urgent
python3 generate.py 600  300  305 urgent
python3 generate.py 40   20   211 urgent
python3 generate.py 100  50   212 urgent
python3 generate.py 300  150  213 urgent
python3 generate.py 600  300  214 urgent

# ========================================================================
# SCENARIO: MIXED
# MIXED — scenario completo e realistico, pronto per local search
# ========================================================================

python3 generate.py 8    5    101 mixed
python3 generate.py 8    5    111 mixed
python3 generate.py 8    5    121 mixed
python3 generate.py 12   8    102 mixed
python3 generate.py 12   8    112 mixed
python3 generate.py 12   8    122 mixed
python3 generate.py 20   13   103 mixed
python3 generate.py 20   13   113 mixed
python3 generate.py 20   13   123 mixed
python3 generate.py 40   20   201 mixed
python3 generate.py 60   30   202 mixed
python3 generate.py 80   40   203 mixed
python3 generate.py 100  50   204 mixed
python3 generate.py 120  60   205 mixed
python3 generate.py 200  100  301 mixed
python3 generate.py 300  150  302 mixed
python3 generate.py 400  200  303 mixed
python3 generate.py 500  250  304 mixed
python3 generate.py 600  300  305 mixed
python3 generate.py 40   20   211 mixed
python3 generate.py 100  50   212 mixed
python3 generate.py 300  150  213 mixed
python3 generate.py 600  300  214 mixed

echo "Tutte le istanze generate con successo."
