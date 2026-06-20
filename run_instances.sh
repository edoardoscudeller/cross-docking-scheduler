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
python3 generate.py 300  150  213 mixed
python3 generate.py 600  300  214 mixed

echo "Tutte le istanze generate con successo."