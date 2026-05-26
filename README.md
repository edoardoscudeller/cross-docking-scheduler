# Cross-Docking Truck Scheduling v0.1

Progetto di ottimizzazione discreta — Advanced Scheduling Systems.  
Università degli Studi di Udine.

---

## Problema

In un centro di cross-docking, un insieme di **camion inbound** (che scaricano merci) e **camion outbound** (che caricano merci) deve essere schedulato su porte condivise.

**Obiettivo:** minimizzare il **makespan**, cioè il tempo totale necessario per completare tutte le operazioni di scarico, trasferimento e carico.

**Vincoli strutturali:**
- una sola porta inbound: i camion inbound scaricano uno alla volta, in sequenza;
- una sola porta outbound: i camion outbound caricano uno alla volta, in sequenza;
- ogni camion inbound `i` ha un **release time** `r[i]`, quindi non può iniziare a scaricare prima del suo arrivo;
- le merci provenienti dal camion inbound `i` richiedono `t[i][j]` unità di tempo per raggiungere il camion outbound `j`;
- il camion outbound `j` può iniziare a caricare solo quando **tutte** le merci necessarie sono disponibili.

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

Una soluzione è rappresentata da due permutazioni:
- `inbound_seq`: ordine di accesso dei camion inbound alla porta di scarico;
- `outbound_seq`: ordine di accesso dei camion outbound alla porta di carico.

---

## Calcolo del makespan

La funzione `ComputeMakespan()` valuta una soluzione data simulando l’intero flusso operativo del magazzino.

### Fase 1 — Sequenza inbound

Per ogni camion inbound `i` nella sequenza `inbound_seq`, si calcola il tempo di completamento dello scarico:

```text
finish_unload[i] = max(available_inbound_door, r[i]) + p[i]
```

Il camion può iniziare solo quando:
- la porta inbound è libera;
- il camion è effettivamente arrivato.

### Fase 2 — Disponibilità delle merci

Per ogni camion outbound `j`, si calcola il momento in cui tutte le merci necessarie sono arrivate nell’area outbound:

```text
goods_ready[j] = max_i ( finish_unload[i] + t[i][j] )
```

### Fase 3 — Sequenza outbound

Per ogni camion outbound `j` nella sequenza `outbound_seq`, si calcola il completamento del carico:

```text
finish[j] = max(available_outbound_door, goods_ready[j]) + q[j]
```

Il camion può iniziare a caricare solo quando:
- la porta outbound è libera;
- tutte le merci sono disponibili.

Il **makespan** finale è:

```text
max_j finish[j]
```

> `ComputeMakespan()` non costruisce la soluzione: si limita a valutarne il costo.

---

## Strategia risolutiva

La versione corrente implementa un solver **greedy** che costruisce separatamente le due sequenze.

### Sequenza inbound — ERT con tie-break SPT

I camion inbound vengono ordinati per:
1. **Earliest Release Time (ERT)**: release time crescente;
2. in caso di parità, **Shortest Processing Time (SPT)**: unload time crescente.

Questa scelta tende a ridurre i tempi morti della porta inbound e a liberarla rapidamente nei casi di parità.

### Sequenza outbound — SPT

I camion outbound vengono ordinati per **load time crescente**.

L’idea è favorire prima i camion più rapidi da completare, riducendo il tempo medio di permanenza sul lato outbound.

### Implementazione logica

```cpp
// Inbound: ERT con tie-break SPT
sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b) {
    if (in.ReleaseTime(a) != in.ReleaseTime(b))
        return in.ReleaseTime(a) < in.ReleaseTime(b);
    return in.UnloadTime(a) < in.UnloadTime(b);
});

// Outbound: SPT
sort(out_seq.begin(), out_seq.end(), [&](unsigned a, unsigned b) {
    return in.LoadTime(a) < in.LoadTime(b);
});
```

---

## Complessità

| Operazione | Complessità |
|---|---|
| Costruzione sequenza inbound | `O(n log n)` |
| Costruzione sequenza outbound | `O(m log m)` |
| Valutazione makespan | `O(n · m)` |

Complessità complessiva del solver greedy con valutazione finale:

```text
O(n log n + m log m + n · m)
```

---

## Limiti attuali

- Le due sequenze sono costruite **indipendentemente**, quindi il greedy non tiene conto in modo esplicito dell’interazione tra ordine inbound e ordine outbound.
- Non ci sono garanzie di ottimalità.
- Il greedy rappresenta una **baseline** semplice e veloce, utile come punto di partenza per confronti futuri con tecniche di miglioramento.

---

## CPU time

La versione corrente misura anche il **tempo totale di esecuzione del programma** per l’istanza caricata.

Il tempo viene misurato nel `main` usando `std::chrono::steady_clock` e include:
- lettura dell’istanza;
- costruzione delle strutture dati;
- esecuzione del solver greedy;
- valutazione finale del makespan.

L’output viene stampato in **secondi**.

Esempio:

```text
Makespan : 57
CPU time : 0.002341 s
```

---

## Build ed esecuzione

Dalla cartella `SourceFiles`:

```bash
cd SourceFiles
make
./CD_Test ../Instances/cd_n004_m003.dzn
```

Per eseguire istanze più grandi:

```bash
./CD_Test ../Instances/cd_n020_m015.dzn
./CD_Test ../Instances/cd_n050_m040.dzn
./CD_Test ../Instances/cd_n120_m100.dzn
```

---

## Formato delle istanze

Le istanze sono memorizzate in formato `.dzn`.

Esempio:

```text
InboundTrucks  = 4;
OutboundTrucks = 3;
ReleaseTime    =;[2][3][4]
UnloadTime     =;[3][5][2]
LoadTime       =;[2][3]
TransferTime   = [| 1,2,1 | 2,1,3 | 1,1,2 | 3,2,1 |];
```

### Significato dei campi

- `InboundTrucks`: numero di camion inbound;
- `OutboundTrucks`: numero di camion outbound;
- `ReleaseTime[i]`: istante di arrivo del camion inbound `i`;
- `UnloadTime[i]`: tempo di scarico del camion inbound `i`;
- `LoadTime[j]`: tempo di carico del camion outbound `j`;
- `TransferTime[i][j]`: tempo necessario a trasferire la merce del camion inbound `i` verso il camion outbound `j`.

La matrice `TransferTime` ha:
- `n` righe;
- `m` colonne.

---

## Generazione automatica delle istanze

La cartella `Instances/` contiene anche uno script Python per generare nuove istanze in modo automatico.

Esempio d’uso:

```bash
cd Instances
python3 generate.py 20 15
python3 generate.py 50 40
python3 generate.py 120 100
```

Lo script genera file con naming coerente, ad esempio:

```text
cd_n020_m015.dzn
cd_n050_m040.dzn
cd_n120_m100.dzn
```

Questo consente di costruire facilmente un insieme di test progressivamente più grande.

---

## Struttura del progetto

```text
cross-docking-scheduler/
├── Instances/
│   ├── cd_n004_m003.dzn
│   ├── cd_n008_m005.dzn
│   ├── cd_n012_m008.dzn
│   ├── cd_n020_m015.dzn
│   ├── cd_n050_m040.dzn
│   ├── cd_n120_m100.dzn
│   └── generate.py
└── SourceFiles/
    ├── CD_Data.hh
    ├── CD_Data.cc
    ├── CD_Greedy.hh
    ├── CD_Greedy.cc
    ├── CD_Driver.cc
    └── Makefile
```

### Descrizione dei file principali

- `CD_Data.hh`: definizione delle classi `CD_Input` e `CD_Output`;
- `CD_Data.cc`: parsing delle istanze `.dzn` e implementazione di `ComputeMakespan()`;
- `CD_Greedy.hh`: dichiarazione del solver greedy;
- `CD_Greedy.cc`: implementazione della costruzione delle sequenze inbound e outbound;
- `CD_Driver.cc`: punto di ingresso del programma, caricamento istanza, esecuzione solver, stampa makespan e CPU time;
- `generate.py`: generatore automatico di nuove istanze `.dzn`.

---

## Versioning del progetto

| Versione | Stato | Contenuto |
|---|---|---|
| v0.0 | passata | Greedy ERT + SPT mono porta |
| **v0.1** | corrente | Greedy ERT + SPT mono porta, CPU time in secondi, istanze estese, generatore Python |
| v0.2 | possibile futura | Greedy migliorato |
| v0.3 | possibile futura | Greedy migliorato multi porta |
| v1.0 | possibile futura | Local Search multi porta |
| v1.1 | possibile futura | Local Search multi porta con istanze più realistiche |



---

## Novità della v0.1 rispetto alla v0.0

- Misurazione del **CPU time** direttamente nel `main`.
- Stampa del tempo di esecuzione in **secondi**.
- Introduzione di un set di **istanze più grandi** oltre alle tre istanze base.
- Naming delle istanze reso più coerente e parametrico, ad esempio `cd_n004_m003.dzn`.
- Aggiunta di `Instances/generate.py` per la **generazione automatica** di nuove istanze `.dzn`.
- Esempi di esecuzione aggiornati per supportare istanze piccole e grandi.