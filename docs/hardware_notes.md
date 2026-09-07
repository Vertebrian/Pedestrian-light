# Hardware notes

## 1. Power domains

La batteria alimenta il modulo boost/power stage. Il boost alimenta il microcontrollore e la striscia LED.

Il pin `K` del modulo è il nodo che consente l'avvio quando viene portato a GND. Non conviene trattarlo come un normale GPIO del microcontrollore: a sistema spento il micro non esiste ancora elettricamente.

## 2. Pulsante

Il pulsante deve avere due ruoli distinti:

- a sistema spento: generare il comando di accensione;
- a sistema acceso: essere letto dal firmware come input utente.

Questi due ruoli non devono entrare in conflitto. Il circuito a transistor serve proprio a fare questa separazione.

## 3. Keep-alive

Dopo il boot il micro deve mantenere attivo il convertitore tramite una linea dedicata.

La cosa elegante è lasciare che il micro sia responsabile solo del mantenimento in vita, non dell'accensione iniziale. In questo modo il ciclo diventa:

```text
OFF
 |
 | pressione
 v
K -> GND
 |
 v
POWER ON
 |
 | MCU boot
 v
KEEP-ALIVE = ON
 |
 v
RUN
 |
 | shutdown command
 v
KEEP-ALIVE = OFF
 |
 v
AUTO CUTOFF
```

## 4. Spegnimento

La soluzione preferita è **non generare necessariamente un nuovo impulso su K** per spegnere.

Se il modulo è progettato per spegnersi quando il keep-alive viene meno, è più semplice e più sicuro fare:

```text
firmware shutdown
       |
       v
keep-alive OFF
       |
       v
boost rileva idle
       |
       v
uscita OFF
```

Il firmware quindi non ha bisogno di essere alimentato per "comandare K" durante l'ultimo istante.

## 5. Verifiche da fare sul banco

Prima del PCB definitivo misurare:

- `K` con sistema OFF;
- `K` durante l'avvio;
- `K` a micro acceso;
- corrente della linea keep-alive;
- corrente del micro in idle/sleep;
- tempo tra keep-alive OFF e spegnimento reale;
- comportamento con striscia LED collegata;
- eventuale reverse current verso il pin K.

Un particolare importante è evitare che la tensione presente sul circuito logico rientri nel nodo K attraverso le giunzioni dei transistor o attraverso i pin del micro.
