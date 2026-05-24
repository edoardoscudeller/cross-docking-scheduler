# Cross-Docking Truck Scheduling

Progetto di ottimizzazione discreta — Advanced Scheduling Systems.  
Università degli Studi di Udine.

---

## Problema

In un centro di cross-docking, un insieme di **camion inbound** (che scaricano merci)
e **camion outbound** (che caricano merci) deve essere schedulato su porte condivise.

**Obiettivo:** minimizzare il **makespan** — il tempo totale per completare tutte le operazioni.

**Vincoli strutturali:**
- 1 sola porta inbound: i camion scaricano uno alla volta, in sequenza
- 1 sola porta outbound: i camion caricano uno alla volta, in sequenza
- Ogni camion inbound `i` ha un **release time** `r[i]`: non può iniziare a scaricare prima di essere arrivato
- Le merci di `i` richiedono `t[i][j]` unità di tempo per raggiungere il camion outbound `j`
- Il camion outbound `j` può iniziare a caricare solo quando **tutte** le sue merci sono disponibili

---

## Modello

| Simbolo | Significato |
|---|---|
| `n` | numero di camion inbound |
| `m` | numero di camion outbound |
| `r[i]` | release time del camion inbound `i` (quando arriva al magazzino) |
| `p[i]` | tempo di scarico del camion inbound `i` |
| `q[j]` | tempo di carico del camion outbound `j` |
| `t[i][j]` | tempo di trasferimento merci da `i` a `j` sul pavimento |

**Soluzione:** due permutazioni — `inbound_seq` e `outbound_seq` — che definiscono
l'ordine di accesso alle rispettive porte.

---

## Come viene calcolato il Makespan

`ComputeMakespan()` simula l'intera operazione al magazzino in tre fasi:

**Fase 1 — Porta inbound** (ciclo su `inbound_seq`):
finish_unload[i] = max(porta_libera, r[i]) + p[i]
Il camion aspetta che la porta sia libera E che lui stesso sia arrivato. Il massimo dei due determina l'inizio effettivo dello scarico.

**Fase 2 — Disponibilità merci per ogni outbound**:
goods_ready[j] = max su tutti gli i di: finish_unload[i] + t[i][j]
Il camion outbound `j` non può iniziare finché l'ultimo pacco necessario non è arrivato.

**Fase 3 — Porta outbound** (ciclo su `outbound_seq`):
finish[j] = max(porta_out_libera, goods_ready[j]) + q[j]

Il camion aspetta sia la porta libera che tutte le merci pronte.

**Makespan** = `max(finish[j])` su tutti i camion outbound.

> `ComputeMakespan` non ottimizza — valuta soltanto il costo di una soluzione data.
> Il solver (greedy o local search) è responsabile di costruire le sequenze.

---

## Versioni

| Versione | Tag | Tecnica | Descrizione |
|---|---|---|---|
| **v0.0** | `v0.0-greedy-ert` | Greedy ERT + SPT | Baseline: sequenze costruite per Earliest Release Time e Shortest Processing Time |
| v0.1 | *(in sviluppo)* | + CPU timing + istanze grandi | Misurazione tempo di computazione e generatore di istanze |
| v1.0 | *(in sviluppo)* | Local Search swap | Miglioramento iterativo tramite mosse di swap sulle sequenze |

---

## v0.0 — Greedy ERT + SPT

### Idea

Il solver greedy costruisce le due sequenze in modo **indipendente**, applicando una regola euristica a ciascuna:

**Sequenza inbound — Earliest Release Time (ERT):**
- I camion vengono ordinati per release time crescente
- Rationale: la porta inbound resta idle il meno possibile
- Tie-break: **Shortest Processing Time (SPT)** sull'unload time — in caso di parità, il camion più veloce da scaricare va prima, liberando la porta prima

**Sequenza outbound — Shortest Processing Time (SPT):**
- I camion vengono ordinati per load time crescente
- Rationale: completare prima i camion veloci minimizza il tempo medio di attesa

### Implementazione

```cpp
// Inbound: ERT con tie-break SPT
sort(in_seq, [](a, b) {
    if (ReleaseTime(a) != ReleaseTime(b))
        return ReleaseTime(a) < ReleaseTime(b);
    return UnloadTime(a) < UnloadTime(b);
});

// Outbound: SPT
sort(out_seq, [](a, b) {
    return LoadTime(a) < LoadTime(b);
});
```

### Complessità

| Operazione | Complessità |
|---|---|
| Costruzione sequenze | `O(n log n + m log m)` |
| Valutazione makespan | `O(n · m)` |

### Limiti

- Le due sequenze sono costruite **indipendentemente**: non si considera l'interazione tra lato inbound e lato outbound
- Nessuna garanzia di ottimalità — è una soluzione di partenza (baseline)
- Il miglioramento richiede una fase di ricerca locale (v1.0)

---

## Build

```bash
cd SourceFiles
make
./CD_Test ../Instances/cd_small.dzn
```

---

## Formato istanza (`.dzn`)

InboundTrucks = 4;
OutboundTrucks = 3;
ReleaseTime =;

UnloadTime = ;
LoadTime = ;
TransferTime = [| 1,2,1 | 2,1,3 | 1,1,2 | 3,2,1 |];


La matrice `TransferTime` ha `n` righe (inbound) e `m` colonne (outbound).  
`TransferTime[i][j]` = tempo per spostare le merci del camion `i` verso il camion `j`.

---

## Struttura del progetto

cross-docking-scheduler/
├── Instances/
│ └── cd_small.dzn # istanza di test (4 inbound, 3 outbound)
└── SourceFiles/
├── CD_Data.hh # CD_Input, CD_Output: strutture dati e interfaccia
├── CD_Data.cc # parsing .dzn e implementazione ComputeMakespan
├── CD_Greedy.hh # dichiarazione GreedyCDSolver
├── CD_Greedy.cc # implementazione solver greedy (ERT + SPT)
├── CD_Driver.cc # main: carica istanza, lancia solver, stampa risultato
└── Makefile