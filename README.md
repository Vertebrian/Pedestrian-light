# Pedestrian Light

ATTiny85 Single-Button Power \& LED Controller
===

Progetto di un piccolo sistema embedded basato su **Digispark / ATtiny85** che usa un solo pulsante per:

* accendere il sistema quando è completamente spento;
* controllare una striscia LED tramite pressione breve;
* riconoscere una pressione lunga di circa 5 s;
* riconoscere una sequenza di più click (configurabile, ad esempio 5 click) per lo spegnimento;
* mantenere alimentato il convertitore boost tramite un segnale di **keep-alive**;
* evitare che il pulsante, quando il microcontrollore è già acceso, interferisca direttamente con il pin **K** del modulo di alimentazione.

L'idea hardware più interessante del progetto è separare logicamente due funzioni che altrimenti entrerebbero in conflitto:

1. il pulsante deve poter tirare **K verso GND** solo per l'avvio;
2. dopo l'avvio, il microcontrollore deve poter mantenere il sistema acceso senza che ogni pressione del pulsante agisca nuovamente su K.

La soluzione studiata usa quindi un piccolo stadio a transistor per **disaccoppiare il pulsante dal controllo K** quando il microcontrollore è operativo.

## Struttura

```text
attiny85\_single\_button\_power\_led/
├── README.md
├── LICENSE
├── .gitignore
├── include/
│   └── config.h
├── src/
│   └── main.cpp
└── docs/
    ├── hardware\_notes.md
    └── transistor\_power\_control.md
```

## Architettura di principio

```text
                 BATTERY
                    |
              +-----+------+
              | BOOST /    |
              | POWER MOD. |
              |             |
              | K <---- control stage <---- ATTiny85
              |             |
              +-------> 5V-ish rail
                         |
                    +----+----+
                    | ATTiny  |
                    | 85      |
                    +----+----+
                         |
             +-----------+-----------+
             |                       |
          BUTTON                 LED STRIP
             |                       |
          control                 DATA
```

> I livelli logici, la polarità del pin K e il comportamento esatto dell'uscita del boost vanno verificati sul modulo reale. Il codice contiene apposta una configurazione per adattare la logica HIGH/LOW.

## Comportamento previsto

* **Sistema spento:** la pressione del pulsante attiva lo stadio che porta K a GND e permette al boost di avviarsi.
* **Boot:** l'ATtiny prende il controllo della propria alimentazione e abilita il keep-alive.
* **Pressione breve:** cambia modalità/colore della striscia LED.
* **5 click rapidi:** richiesta di spegnimento.
* **Pressione lunga \~5 s:** richiesta di spegnimento.
* **Spegnimento:** il firmware disabilita il keep-alive e lascia che il modulo torni nello stato di idle/cutoff.

## Perché non comandare K direttamente dal micro?

Perché quando il sistema è spento l'ATtiny non è alimentato e quindi non può essere lui a generare il primo comando di accensione. Inoltre, se il pulsante fosse collegato direttamente a K, la stessa pressione usata come input utente potrebbe continuare a pilotare K anche a micro acceso.

La soluzione più robusta è quindi utilizzare il pulsante come comando di boot e un piccolo circuito a transistor per trasferire il controllo di K al microcontrollore dopo l'avvio.

## Nota sul software

Il codice è volutamente semplice e portabile. Non assume in modo rigido il modello della striscia LED: il punto di integrazione per una striscia addressable (ad esempio WS2812) è commentato in `src/main.cpp`.

Per usare una libreria specifica basta sostituire la funzione `setLedPattern()`.

## Stato del progetto

Questo repository raccoglie il concept hardware/software e una base firmware da adattare al circuito definitivo. Prima della realizzazione PCB è necessario verificare con il multimetro e con il modulo reale:

* polarità e soglia del pin K;
* corrente richiesta dal modulo in standby;
* tensione effettiva disponibile all'ATtiny85;
* tempi di auto-spegnimento del boost;
* livelli di tensione prodotti dallo stadio a transistor;
* assorbimento totale a riposo.

## Possibili evoluzioni

* sleep dell'ATtiny85 durante l'inattività;
* debounce hardware del pulsante;
* rilevamento click più raffinato;
* salvataggio dell'ultima modalità LED in EEPROM;
* controllo luminosità PWM;
* misura della tensione batteria;
* protezione da sottotensione;
* PCB integrato con connettore LED e pulsante.

