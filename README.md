# Cross-Docking Truck Scheduling — v0.0

Progetto di ottimizzazione discreta — Advanced Scheduling Systems.  
Università degli Studi di Udine.

---

## Problema

In un centro di cross-docking, un insieme di **camion inbound** (che scaricano merci)
e **camion outbound** (che caricano merci) deve essere schedulato su porte condivise.

**Obiettivo:** minimizzare il **makespan** — il tempo totale per completare tutte le operazioni.

**Vincoli:**
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
| `r[i]` | release time del camion inbound `i` |
| `p[i]` | tempo di scarico del camion inbound `i` |
| `q[j]` | tempo di carico del camion outbound `j` |
| `t[i][j]` | tempo di trasferimento merci da `i` a `j` |

**Soluzione:** due permutazioni — `inbound_seq` e `outbound_seq` — che definiscono
l'ordine di accesso alle rispettive porte.

---

## Calcolo del Makespan

`ComputeMakespan()` simula l'intera operazione al magazzino in tre fasi:

**Fase 1 — Porta inbound** (ciclo su `inbound_seq`):

finish_unload[i] = max(porta_libera, r[i]) + p[i]
Il camion aspetta che la porta sia libera E che lui stesso sia arrivato.

**Fase 2 — Disponibilità merci per ogni outbound**:

goods_ready[j] = max su tutti gli i di: finish_unload[i] + t[i][j]
Il camion outbound `j` aspetta l'ultimo pacco — quello che arriva per ultimo.

**Fase 3 — Porta outbound** (ciclo su `outbound_seq`):

finish[j] = max(porta_out_libera, goods_ready[j]) + q[j]

**Makespan** = `max(finish[j])` su tutti i camion outbound.

> `ComputeMakespan` non ottimizza — valuta soltanto il costo di una soluzione data.
> Il solver greedy è responsabile di costruire le sequenze.

---

## Tecnica — Greedy ERT + SPT

Il solver costruisce le due sequenze in modo **indipendente**:

**Sequenza inbound — Earliest Release Time (ERT):**
- I camion vengono ordinati per release time crescente
- Rationale: la porta inbound resta idle il meno possibile
- Tie-break: **Shortest Processing Time (SPT)** — in caso di parità sul release time,
  il camion con unload time minore va prima, liberando la porta prima

**Sequenza outbound — Shortest Processing Time (SPT):**
- I camion vengono ordinati per load time crescente
- Rationale: completare prima i camion veloci minimizza il tempo medio di attesa

### Complessità

| Operazione | Complessità |
|---|---|
| Costruzione sequenze | `O(n log n + m log m)` |
| Valutazione makespan | `O(n · m)` |

### Limiti

- Le due sequenze sono costruite **indipendentemente**: non si considera l'interazione tra lato inbound e lato outbound
- Nessuna garanzia di ottimalità — è una soluzione baseline

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


`TransferTime[i][j]` = tempo per spostare le merci del camion inbound `i` verso il camion outbound `j`.

---

## Struttura del progetto

cross-docking-scheduler/
├── Instances/
│ └── cd_small.dzn # istanza di test (4 inbound, 3 outbound)
└── SourceFiles/
├── CD_Data.hh # CD_Input, CD_Output: strutture dati e interfaccia
├── CD_Data.cc # parsing .dzn e implementazione ComputeMakespan
├── CD_Greedy.hh # dichiarazione GreedyCDSolver
├── CD_Greedy.cc # implementazione greedy ERT + SPT
├── CD_Driver.cc # main: carica istanza, lancia solver, stampa risultato
└── Makefile

