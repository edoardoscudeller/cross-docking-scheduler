# Cross-Docking Truck Scheduling v0.2 — Greedy avanzato multi‑porta

Progetto di ottimizzazione discreta — Advanced Scheduling Systems.  
Università degli Studi di Udine.

---

## Problema

In un centro di cross-docking, un insieme di **camion inbound** (che scaricano merci) e **camion outbound** (che caricano merci) deve essere schedulato su **più porte inbound e outbound**, condivise tra tutti i camion.[page:1]

**Obiettivo:** minimizzare il **makespan**, cioè il tempo totale necessario per completare tutte le operazioni di scarico, trasferimento e carico, tenendo conto di:

- release time dei camion inbound;
- tempi di trasferimento tra area inbound e outbound;
- tempi di servizio a porta;
- numero limitato di porte inbound/outbound (multi-door).[page:1][page:0]

---

## Modello

Il modello di base è lo stesso della v0.1 (mono porta), con in più il numero di porte:

| Simbolo | Significato |
|---|---|
| `n` | numero di camion inbound |
| `m` | numero di camion outbound |
| `d_in` | numero di porte inbound |
| `d_out` | numero di porte outbound |
| `r[i]` | release time del camion inbound `i` |
| `p[i]` | tempo di scarico del camion inbound `i` |
| `q[j]` | tempo di carico del camion outbound `j` |
| `t[i][j]` | tempo di trasferimento merci da `i` a `j` |

Una soluzione è descritta da:

- una **sequenza inbound** `inbound_seq` (permutazione dei camion inbound);
- una **sequenza outbound** `outbound_seq` (permutazione dei camion outbound);
- un’assegnazione implicita alle porte inbound/outbound generata dal solver secondo una politica EAD (Earliest Available Door).[page:0]

---

## Calcolo del makespan (multi‑porta)

La funzione `ComputeMakespan()` valuta una soluzione completa (sequenze + assegnazioni porte) simulando:

1. **Scarico inbound su `d_in` porte parallele**

   Per ogni camion inbound `i` nella sequenza `inbound_seq`:

   - si sceglie la porta inbound che si libera prima (politica EAD);
   - si calcola il tempo di fine scarico tenendo conto sia del release time sia della disponibilità della porta.[page:0]

   In pseudocodice:

   ```text
   best_d      = argmin_d door_free_in[d]
   start_i     = max(door_free_in[best_d], r[i])
   finish_unload[i] = start_i + p[i]
   door_free_in[best_d] = finish_unload[i]
   ```

2. **Disponibilità delle merci (`goods_ready`)**

   Per ogni camion outbound `j`, si calcola quando tutte le merci necessarie sono arrivate sull’area outbound:[page:0]

   ```text
   goods_ready[j] = max_i ( finish_unload[i] + t[i][j] )
   ```

3. **Carico outbound su `d_out` porte parallele**

   Dato l’ordine `outbound_seq`, ogni camion outbound è assegnato dinamicamente alla porta outbound che si libera prima (EAD):[page:0]

   ```text
   best_d      = argmin_d door_free_out[d]
   start_j     = max(door_free_out[best_d], goods_ready[j])
   finish[j]   = start_j + q[j]
   door_free_out[best_d] = finish[j]
   ```

Il **makespan** è il massimo tra tutti i `finish[j]`.[page:0]

---

## Strategia risolutiva — Greedy composito v0.2

La v0.2 sostituisce il semplice ERT+SPT della v0.1 con un **greedy avanzato “composite-score”** e una costruzione **NEH‑style** per la sequenza outbound, entrambi adattati al caso multi‑porta.[page:0]

### Fase 1 — Sequenza inbound con punteggio composito

La sequenza inbound è costruita ordinando i camion secondo un **punteggio composito** che combina:

- release time `r[i]`;
- tempo di scarico `p[i]`;
- un termine di **impatto pesato** sul lato outbound, calcolato come media pesata dei tempi di trasferimento verso i vari outbound, pesati con i tempi di carico:[page:0]

```text
weighted_impact(i) = 
    ( Σ_j t[i][j] · q[j] ) / ( Σ_j t[i][j] )  se Σ_j t[i][j] > 0
    0                                         altrimenti
```

Il punteggio complessivo è:[page:0]

```text
score(i) = r[i] + p[i] + weighted_impact(i)
```

Per non perdere la natura “earliest release” del problema, il confronto tra due camion `(a,b)` usa una **guardia ERT conservativa**:

- si calcola la media dei tempi di scarico `[avg_unload]`;
- se la differenza di release time tra `a` e `b` è “grande” rispetto ad `avg_unload`, prevale l’ERT;
- altrimenti si confrontano i punteggi compositi.[page:0]

In codice (semplificato):

```cpp
// Precompute weighted_impact[i]
vector<double> weighted_impact(n);
// ...

// Score composito
vector<double> score(n);
for (unsigned i = 0; i < n; i++)
    score[i] = in.ReleaseTime(i) + in.UnloadTime(i) + weighted_impact[i];

// Soglia conservativa ERT
double avg_unload = /* media di p[i] */;

sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b) {
    double ert_diff = in.ReleaseTime(a) - in.ReleaseTime(b);
    if (std::abs(ert_diff) > avg_unload)
        return in.ReleaseTime(a) < in.ReleaseTime(b);  // ERT
    return score[a] < score[b];                        // composite score
});
```

Questa fase:

- mantiene la struttura ERT della v0.1;
- ma “rompe i pareggi intelligenti” privilegiando camion con maggiore impatto sul lato outbound (weighted impact).[page:0]

### Fase 2 — Simulazione inbound con `d_in` porte (multi‑door EAD)

Una volta fissata `inbound_seq`, il solver simula lo scarico su **`d_in` porte parallele** usando una politica **EAD (Earliest Available Door)**:

- per ogni camion nella sequenza, sceglie la porta inbound con `door_free_in[d]` minimo;
- aggiorna `finish_unload[i]` e il tempo di liberazione della porta.[page:0]

Estratto di implementazione:

```cpp
vector<unsigned> finish_unload(n, 0);
vector<unsigned> door_free_in(in.InboundDoors(), 0);
vector<unsigned> assigned_door_in(n, 0);

for (unsigned pos = 0; pos < n; pos++) {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++)
        if (door_free_in[d] < door_free_in[best_door])
            best_door = d;

    unsigned i = in_seq[pos];
    finish_unload[i] = max(door_free_in[best_door], in.ReleaseTime(i))
                     + in.UnloadTime(i);
    door_free_in[best_door] = finish_unload[i];
    assigned_door_in[pos]   = best_door;
}

out.SetInboundSeq(in_seq);
out.SetInboundDoor(assigned_door_in);
```

Questa parte è la principale estensione per la **multi-door inbound** rispetto alla v0.1 (mono porta).[page:0]

### Fase 3 — Sequenza outbound NEH‑style con `d_out` porte

La sequenza outbound è costruita con una logica in due passi:

1. **Ranking iniziale dei camion outbound**

   Si costruisce un ranking decrescente rispetto a:

   ```text
   score_out(j) = goods_ready[j] + q[j]
   ```

   cioè somma tra momento di disponibilità delle merci e tempo di carico, in analogia con la prima fase dell’algoritmo NEH per flow shop.[page:0]

   ```cpp
   vector<unsigned> ranked(m);
   iota(ranked.begin(), ranked.end(), 0);

   sort(ranked.begin(), ranked.end(), [&](unsigned a, unsigned b) {
       unsigned score_a = goods_ready[a] + in.LoadTime(a);
       unsigned score_b = goods_ready[b] + in.LoadTime(b);
       return score_a > score_b;  // decrescente
   });
   ```

2. **Inserimento iterativo (NEH adaptato, multi‑porta)**

   Partendo da una sequenza vuota `out_seq`, si inserisce ogni camion `ranked[step]` nella posizione che minimizza il **makespan outbound parziale**, calcolato chiamando una funzione di valutazione dedicata:[page:0]

   ```cpp
   for (unsigned step = 0; step < m; step++) {
       unsigned best_pos      = 0;
       unsigned best_makespan = numeric_limits<unsigned>::max();

       for (unsigned pos = 0; pos <= out_seq.size(); pos++) {
           vector<unsigned> candidate;
           candidate.reserve(out_seq.size() + 1);

           // inserimento in posizione pos
           for (unsigned k = 0; k < pos; k++)
               candidate.push_back(out_seq[k]);
           candidate.push_back(ranked[step]);
           for (unsigned k = pos; k < out_seq.size(); k++)
               candidate.push_back(out_seq[k]);

           unsigned ms = ComputePartialOutboundMakespan(in, finish_unload, candidate);
           if (ms < best_makespan) {
               best_makespan = ms;
               best_pos      = pos;
           }
       }

       out_seq.insert(out_seq.begin() + best_pos, ranked[step]);
   }

   out.SetOutboundSeq(out_seq);
   ```

#### Valutazione parziale outbound (multi‑porta)

La funzione ausiliaria `ComputePartialOutboundMakespan()` simula il solo lato outbound su `d_out` porte parallele, con politica EAD, per una sequenza parziale:[page:0]

```cpp
static unsigned ComputePartialOutboundMakespan(
    const CD_Input&  in,
    const vector<unsigned>& finish_unload,
    const vector<unsigned>& partial_seq
) {
    vector<unsigned> door_free(in.OutboundDoors(), 0);
    unsigned makespan = 0;

    for (unsigned pos = 0; pos < partial_seq.size(); pos++) {
        unsigned j = partial_seq[pos];

        unsigned goods_ready = 0;
        for (unsigned i = 0; i < in.InboundTrucks(); i++)
            goods_ready = max(goods_ready, finish_unload[i] + in.TransferTime(i, j));

        unsigned best_door = 0;
        for (unsigned d = 1; d < in.OutboundDoors(); d++)
            if (door_free[d] < door_free[best_door])
                best_door = d;

        door_free[best_door] =
            max(door_free[best_door], goods_ready) + in.LoadTime(j);

        makespan = max(makespan, door_free[best_door]);
    }

    return makespan;
}
```

Questa funzione è la **chiave** dell’heuristic NEH‑style: consente di valutare rapidamente l’effetto di inserire un camion outbound in ogni posizione possibile, tenendo conto di `d_out` porte e dei tempi di trasferimento.[page:0]

---

## Output e assegnazione alle porte

Il `main` (`CD_Driver.cc`) stampa, per istanze piccole (`n ≤ 20`):

- input completo (inclusi `ReleaseTime` e parametri multi‑porta);
- sequenza originale ordinata per release time;
- sequenze inbound/outbound greedy risultanti;
- makespan finale;
- numero di porte inbound e outbound utilizzate;
- CPU time in secondi.[page:1]

Estratto:

```cpp
if (in.InboundTrucks() <= 20) {
    vector<unsigned> orig(in.InboundTrucks());
    iota(orig.begin(), orig.end(), 0);
    sort(orig.begin(), orig.end(), [&](unsigned a, unsigned b){
        return in.ReleaseTime(a) != in.ReleaseTime(b)
            ? in.ReleaseTime(a) < in.ReleaseTime(b)
            : a < b;
    });

    cout << "Original sequence: ";
    // ...
    cout << out << endl;
}

unsigned makespan = out.ComputeMakespan();
auto t_end  = chrono::steady_clock::now();
double elapsed_s = chrono::duration<double>(t_end - t_start).count();

cout << "Makespan : " << makespan << endl;
cout << "Doors : in=" << in.InboundDoors()
     << " out=" << in.OutboundDoors() << endl;
cout << fixed << setprecision(6);
cout << "CPU time : " << elapsed_s << " s" << endl;
```

---

## Complessità

Rispetto alla v0.1, la complessità cambia soprattutto lato outbound:

- costruzione punteggio composito inbound: `O(n · m)` per `weighted_impact` + `O(n log n)` per l’ordinamento;
- simulazione inbound multi‑porta: `O(n · d_in)` (con `d_in` tipicamente piccolo);
- ranking outbound iniziale: `O(m log m)`;
- fase NEH‑style outbound: per ogni inserimento si provano tutte le posizioni, e per ogni candidato si valuta una simulazione `ComputePartialOutboundMakespan` di costo `O(n · d_out)`:[page:0]

  ```text
  O( m^2 · n · d_out )
  ```

In pratica, `d_in` e `d_out` sono piccoli (poche porte), quindi il costo principale è il termine `O(m^2 · n)` della costruzione NEH‑style outbound.[page:0]

---

## Novità principali della v0.2 rispetto alla v0.1

- **Multi-door inbound e outbound**: simulazione esplicita di `d_in` e `d_out` porte con politica **Earliest Available Door** su entrambi i lati.[page:0]
- **Sequenza inbound avanzata**:
  - passaggio da semplice ERT+SPT a **punteggio composito** che combina release time, tempi di scarico e un termine di impatto pesato sui trasferimenti.[page:0]
  - soglia ERT “robusta” basata sulla media dei tempi di scarico.
- **Sequenza outbound NEH‑style**:
  - ranking iniziale basato su `goods_ready[j] + q[j]`;
  - inserimento iterativo con valutazione del **makespan parziale** mediante `ComputePartialOutboundMakespan`, adattato al cross-dock multi‑porta.[page:0]
- **Output arricchito**:
  - stampa del numero di porte inbound/outbound utilizzate;
  - assegnazione esplicita alle porte salvata in `CD_Output` per scopi di debug/analisi.[page:0][page:1]

---

## Build, run e generazione istanze (multi‑scenario)

Il flusso di build rimane invariato:

```bash
cd SourceFiles
make
./CD_Test ../Instances/<istanza>.dzn
```

Sono inoltre disponibili:

- script per la **generazione automatica di istanze multi-scenario** (uniform, sparse, clustered, asymmetric, congested, urgent, mixed);
- script `run_all.sh` per lanciare automaticamente il solver su tutte le istanze e produrre un file di risultati aggregato.[file:25][file:11]

> Nella v0.2 il focus principale è il **miglioramento del greedy** (punteggio composito e costruzione NEH‑style) e l’estensione al caso **multi‑porta**, mentre il supporto multi‑door sarà retro‑integrato anche nella v0.1 come baseline per i confronti.