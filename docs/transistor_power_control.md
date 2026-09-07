# Stadio a transistor per disaccoppiare il pulsante dal pin K

## Problema

Il problema originale è questo:

```text
                 BUTTON
                    |
                    +------> K
                    |
                    +------> MCU input
```

Quando il sistema è OFF, il pulsante deve poter portare `K` a GND per accendere il modulo.

Quando il sistema è ON, però, lo stesso pulsante non dovrebbe continuare ad avere il potere di spegnere il boost solamente perché viene premuto per cambiare il colore della striscia LED.

## Idea

Si aggiunge un elemento attivo tra pulsante e K.

La logica desiderata è:

```text
                MCU
                 |
           disable / isolate
                 |
BUTTON ---> transistor stage ---> K
```

Durante il boot lo stadio permette il comando K. Dopo il boot il micro forza lo stadio in una condizione in cui il pulsante viene visto **solo come input**, mentre K non viene più pilotato accidentalmente.

## Possibile implementazione con BJT

Dato che si dispone di molti BJT, si può costruire una soluzione discreta con due transistor complementari o due NPN, a seconda della topologia scelta.

La ragione per cui questa soluzione è interessante è la disponibilità dei componenti e il bassissimo costo. Non serve necessariamente un MOSFET se le correnti sul nodo K sono piccole.

### Principio con due transistor

Un'architettura tipica è:

```text
                  Vlogic
                    |
                  Rpull
                    |
                    +------ drive node
                    |
                 Q1 / Q2
                    |
BUTTON ------------+
                    |
                    +------------ K

MCU -------------------------- control
```

Il primo transistor può essere usato per abilitare/disabilitare il percorso del pulsante; il secondo per pilotare il nodo K con una corrente limitata e con un isolamento migliore.

La scelta concreta dipende da:

- tensione della batteria;
- tensione logica disponibile;
- soglia del modulo sul pin K;
- corrente richiesta per portare K a GND;
- corrente di leakage accettabile.

## Perché due transistor possono essere più comodi

Con un solo transistor è possibile ottenere la commutazione, ma può essere difficile ottenere contemporaneamente:

1. comando affidabile di K;
2. isolamento del pulsante quando il sistema è acceso;
3. livelli logici puliti;
4. basso consumo;
5. comportamento prevedibile durante il boot.

Con due transistor è più facile separare le funzioni:

```text
Q1 = sensing / enable
Q2 = actual K pull-down
```

Questa separazione rende il circuito più semplice da ragionare e da diagnosticare con il multimetro.

## Attenzione alla polarizzazione

Un punto già emerso durante le prove è che una base può trovarsi a una tensione apparentemente "strana" rispetto alla sorgente che la pilota. Non basta guardare la tensione base rispetto alla batteria: bisogna guardare sempre la differenza:

```text
V_BE = V_B - V_E
```

Un BJT NPN conduce quando `V_BE` è sufficientemente positiva. Quindi una base a qualche centinaio di millivolt non significa automaticamente che il transistor sia spento: bisogna sapere dove si trova l'emettitore in quel preciso istante.

## Resistenze

Le resistenze non vanno scelte solamente in base a valori "comodi".

Bisogna considerare:

```text
Ib = (Vdrive - Vbe) / Rb
Ic = beta * Ib
```

con il limite pratico che, quando il transistor deve saturare, è preferibile progettare usando un `forced beta` conservativo anziché il beta nominale del datasheet.

Per un semplice controllo logico a bassissima corrente, valori nell'ordine di qualche kOhm o decine di kOhm possono essere ragionevoli, ma il valore finale va verificato sul circuito reale.

## Perché 47 kOhm sul pulsante può avere senso

Un pull-up da 47 kOhm è un compromesso tra:

- corrente di riposo molto bassa;
- immunità al rumore ancora ragionevole;
- ingresso definito quando il pulsante è aperto.

Con 3.7 V, un pull-up da 47 kOhm assorbe circa:

```text
I = 3.7 V / 47 kOhm ~= 79 uA
```

Un 10 kOhm porterebbe la corrente a circa 370 uA quando il pulsante chiude il percorso verso massa.

Se il nodo è lungo, rumoroso o attraversa un ambiente elettricamente sporco, 10 kOhm può però offrire una maggiore immunità. La scelta dipende quindi dal compromesso consumo/robustezza.

## MOSFET vs BJT

### MOSFET

Vantaggi:
- gate praticamente senza corrente statica;
- molto adatto al low-power;
- ottimo per fare un semplice pull-down su K;
- meno dipendenza dal guadagno di corrente.

Svantaggi:
- serve scegliere correttamente il dispositivo rispetto alla tensione di gate;
- il circuito di boot può diventare meno intuitivo se gate e source non sono sullo stesso riferimento.

### BJT

Vantaggi:
- economici e disponibili;
- semplici per piccoli segnali;
- ottimi se la corrente di K è molto bassa.

Svantaggi:
- corrente di base;
- V_BE non nulla;
- dipendenza dal guadagno e dalla saturazione;
- maggiore attenzione richiesta alle resistenze.

Per questo prototipo, usare i BJT già disponibili è assolutamente sensato. Per il PCB definitivo, però, vale la pena confrontare la soluzione con un piccolo MOSFET logic-level.

## Strategia di spegnimento consigliata

La strategia software/hardware preferita rimane:

```text
5 click oppure 5 s
       |
       v
shutdownSystem()
       |
       +--> LED OFF
       |
       +--> KEEP-ALIVE OFF
       |
       v
boost -> IDLE/CUTOFF
       |
       v
MCU perde alimentazione
```

Questo evita di dover creare una seconda complessa sequenza per "premere K" artificialmente durante lo spegnimento.

## Nota sul prototipo

Prima di fissare definitivamente lo schema è opportuno misurare il comportamento del modulo in tre condizioni:

1. `K = GND`;
2. `K` flottante;
3. `K` pilotato dal transistor.

La misura deve essere fatta sia a vuoto sia con il carico reale collegato.
