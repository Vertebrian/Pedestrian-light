#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/wdt.h>  // <<<< AGGIUNTO

#define PIN_ON_COUPLER     5
#define PIN_LEDS   0
#define PIN_BTN    2
#define NUM_LEDS   12
#define BRIGHT     230     // ~90 % luminosità (max 255)

#define HOLD2_MS   2000UL
#define SHOW_SELECTED_MS 1000UL
#define ALERT_TOTAL_MS    15000UL
#define MULTICLICK_GAP_MS 300UL
#define DEBOUNCE_MS       30UL

// velocità: lento, medio, veloce
const uint16_t blinkOn[3]  = {200,150,80};
const uint16_t blinkOff[3] = {200,150,80};
const uint16_t fillStep[3] = {60,40,25};

Adafruit_NeoPixel strip(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);

// colori
const uint32_t palette[] = {
  0x00C8B0,0x00FF00,0x950EFE,0xFF80B4,0xFF9300,0xFFD700,0xFFFFFF,
  Adafruit_NeoPixel::Color(255,120,40)
};
const uint8_t N_COL = sizeof(palette)/sizeof(palette[0]);

// stato
uint8_t colorIdx = 0;
uint8_t speedLevel = 1;

struct Btn {
  bool lastStable = true;
  bool lastRaw = true;
  uint32_t lastChangeMs = 0;
  uint32_t pressedAt = 0;
  uint8_t clickCount = 0;
} btn;
bool btnFallingEdge=false, btnRisingEdge=false;

inline bool btnReadRaw(){ return digitalRead(PIN_BTN)==LOW; }

// ---------- safeDelay: aspetta SENZA “addormentarsi” ----------
static inline void safeDelay(uint16_t ms){
  uint32_t t0 = millis();
  while(millis() - t0 < ms){
    buttonUpdate();
    wdt_reset();              // alimenta il watchdog
    delayMicroseconds(500);   // micro-pausa breve
  }
}

// ---------------- BUTTON ----------------
void buttonUpdate(){
  bool raw=btnReadRaw();
  uint32_t now=millis();
  if(raw!=btn.lastRaw){ btn.lastRaw=raw; btn.lastChangeMs=now; }
  btnFallingEdge=btnRisingEdge=false;
  if(now-btn.lastChangeMs>=DEBOUNCE_MS){
    if(raw!=btn.lastStable){
      btn.lastStable=raw;
      if(raw){ btnFallingEdge=true; btn.pressedAt=now; }
      else { btnRisingEdge=true; btn.clickCount++; }
    }
  }
}
inline bool checkHold(uint32_t dur){ return btn.lastStable && (millis()-btn.pressedAt>=dur); }

// ---------------- LED HELPERS ----------------
void ledsOff(){ strip.clear(); strip.show(); } //provo a disattivare lo strip show
void fillColor(uint32_t c){ for(uint16_t i=0;i<NUM_LEDS;i++) strip.setPixelColor(i,c); strip.show(); }

void progressiveFill(uint32_t c,uint16_t step){
  strip.clear();
  for(uint16_t i=0;i<NUM_LEDS;i++){
    strip.setPixelColor(i,c); strip.show();
    uint32_t t0=millis();
    while(millis()-t0<step){
      buttonUpdate();
      if(btnFallingEdge) return;
      wdt_reset();
      safeDelay(1); // << sostituisce delay(1)
    }
  }
}

// ---------------- SEQUENZA ----------------
void runAttentionSequence(uint32_t totalMs,uint32_t color){
  uint32_t start=millis();
  while(millis()-start<totalMs){
    for(uint8_t k=0;k<2;k++){ // 2 lampeggi
      fillColor(color);
      uint32_t t0=millis();
      while(millis()-t0<blinkOn[speedLevel]){
        buttonUpdate(); if(btnFallingEdge){ledsOff();return;}
        wdt_reset();
        safeDelay(1);
      }
      ledsOff(); t0=millis();
      while(millis()-t0<blinkOff[speedLevel]){
        buttonUpdate(); if(btnFallingEdge){ledsOff();return;}
        wdt_reset();
        safeDelay(1);
      }
    }
    for(uint8_t k=0;k<2;k++){ // 2 riempimenti progressivi
      progressiveFill(color,fillStep[speedLevel]);
      if(btnFallingEdge){ ledsOff(); return; }
      uint32_t t0=millis();
      while(millis()-t0<blinkOn[speedLevel]){
        buttonUpdate(); if(btnFallingEdge){ledsOff();return;}
        wdt_reset();
        safeDelay(1);
      }
      ledsOff(); t0=millis();
      while(millis()-t0<blinkOff[speedLevel]){
        buttonUpdate(); if(btnFallingEdge){ledsOff();return;}
        wdt_reset();
        safeDelay(1);
      }
    }
  }
  ledsOff();
}
// ---------------SEQUENZA CONTINUA-------
// Lampeggio continuo: resta attivo finché non riceve un nuovo click.
// Usa i profili di velocità definiti in blinkOn[] / blinkOff[] e speedLevel.
void runAttentionContinuous(uint32_t color) {
  // facoltativo: azzera eventuali conteggi residui
  btn.clickCount = 0;

  for (;;) {
    // --- fase ON ---
    fillColor(color);
    uint32_t t0 = millis();
    uint16_t onMs  = blinkOn[speedLevel];   // dipende dal profilo corrente
    while (millis() - t0 < onMs) {
      buttonUpdate();
      if (btnRisingEdge) { ledsOff(); return; } // interrompi al primo click
      wdt_reset();
      safeDelay(1);
    }

    // --- fase OFF ---
    ledsOff();
    uint32_t t1 = millis();
    uint16_t offMs = blinkOff[speedLevel];
    while (millis() - t1 < offMs) {
      buttonUpdate();
      if (btnRisingEdge) { ledsOff(); return; } // interrompi al primo click
      wdt_reset();
      safeDelay(1);
    }

    // Nota: se durante il ciclo fai 3 click (fuori da qui) per cambiare speedLevel,
    // al giro successivo i tempi on/off useranno il nuovo profilo automaticamente.
  }
}

// ---------------- SETUP ----------------
void setup(){
  // --- Watchdog robusto all'avvio ---
  MCUSR &= ~(1 << WDRF); // cancella flag reset da WDT
  wdt_disable();         // disabilita mentre parte
  wdt_enable(WDTO_1S);   // riabilita con timeout ~1s

  pinMode(PIN_LEDS,OUTPUT); digitalWrite(PIN_LEDS,LOW);
  pinMode(PIN_BTN,INPUT_PULLUP);
  strip.begin(); strip.setBrightness(BRIGHT); ledsOff();

  // Piccolo ritardo di avvio per stabilizzare Vcc
  safeDelay(100);
}

// ---------------- LOOP ----------------
void loop(){
  buttonUpdate();

  // hold 2 s → luce fissa 10 min
  if(checkHold(HOLD2_MS)){
    uint32_t tEnd=millis()+600000UL;
    fillColor(palette[colorIdx]);
    while(millis()<tEnd){
      buttonUpdate(); if(btnFallingEdge) break;
      wdt_reset();
      safeDelay(1);
    }
    ledsOff(); btn.clickCount=0; return;
  }

  // click multipli
  if(btnRisingEdge){
    uint32_t start=millis();
    while(millis()-start<MULTICLICK_GAP_MS){
      buttonUpdate();
      if(btnRisingEdge) start=millis();
      wdt_reset();
      safeDelay(1);
    }
    uint8_t n=btn.clickCount; btn.clickCount=0;

    if(n==1){
      runAttentionSequence(ALERT_TOTAL_MS,palette[colorIdx]);
      ledsOff();
    }
    else if(n==2){
      colorIdx=(colorIdx+1)%N_COL;
      fillColor(palette[colorIdx]);
      uint32_t t0=millis();
      while(millis()-t0<SHOW_SELECTED_MS){
        buttonUpdate();
        wdt_reset();
        safeDelay(1);
      }
      ledsOff();
    }
    else if(n==3){
     // NEW: lampeggio continuo (stop al prossimo click)
    runAttentionContinuous(palette[colorIdx]);
    ledsOff();
      }
    
    else if(n==4){
      speedLevel=(speedLevel+1)%3;
      // feedback: 1–3 flash per indicare livello
      for(uint8_t i=0;i<=speedLevel;i++){
        fillColor(palette[colorIdx]); safeDelay(150);
        ledsOff(); safeDelay(120);
      }
    }
  
  }
  safeDelay(1);
}
