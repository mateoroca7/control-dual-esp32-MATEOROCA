
const int pot1Pin = 34;
const int pot2Pin = 35;
const int led1Pin = 25;
const int led2Pin = 26;
const int btnModo = 32;
const int btnPwr  = 33;

#define LEDC_FREQ_HZ   5000
#define LEDC_BITS      8  

volatile bool timerFlag = false;   
volatile bool estadoSistema  = true;    
volatile int  modoActual = 1;      
volatile unsigned long lastBtnModo = 0;
volatile unsigned long lastBtnPwr = 0;
#define DEBOUNCE_US 250000UL      
volatile bool estadoLed2 = false;

hw_timer_t* timer = NULL;


void IRAM_ATTR onTimer() {
  timerFlag = true;
}

void IRAM_ATTR ISR_CambiarModo() {
  unsigned long ahora = micros();
  if (ahora - lastBtnModo > DEBOUNCE_US) {
    lastBtnModo = ahora;
    if (estadoSistema) {
      modoActual = (modoActual == 1) ? 2 : 1;
    }
  }
}

void IRAM_ATTR ISR_ToggleSistema() {
  unsigned long ahora = micros();
  if (ahora - lastBtnPwr > DEBOUNCE_US) {
    lastBtnPwr = ahora;
    estadoSistema = !estadoSistema;
  }
}

void configurarTimer(uint32_t freqHz) {
  if (freqHz < 1) freqHz = 1;
  
  if (timer != NULL) {
    timerStop(timer); 
  } else {
    timer = timerBegin(1000000); 
    timerAttachInterrupt(timer, &onTimer);
  }

  uint64_t ticks = 1000000UL / (freqHz * 2);
  timerAlarm(timer, ticks, true, 0);
  timerWrite(timer, 0); 
  timerStart(timer);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(btnModo, INPUT_PULLUP);
  pinMode(btnPwr, INPUT_PULLUP);

 
  ledcAttach(led1Pin, LEDC_FREQ_HZ, LEDC_BITS);

  attachInterrupt(digitalPinToInterrupt(btnModo), ISR_CambiarModo, FALLING);
  attachInterrupt(digitalPinToInterrupt(btnPwr), ISR_ToggleSistema, FALLING);

  configurarTimer(2);
  Serial.println("=== SISTEMA CORREGIDO Y LISTO ===");
}

void loop() {

  if (!estadoSistema) {
    ledcWrite(led1Pin, 0);
    digitalWrite(led2Pin, LOW);
    
    static unsigned long lastOffMsg = 0;
    if (millis() - lastOffMsg > 1000) {
      Serial.println("SISTEMA DESACTIVADO");
      lastOffMsg = millis();
    }
    return;e
  }

  if (modoActual == 1) {
    digitalWrite(led2Pin, LOW);
    int valorADC = analogRead(pot1Pin);
    int valorPWM = map(valorADC, 0, 4095, 0, 255);
    ledcWrite(led1Pin, valorPWM);

    static unsigned long lastLog1 = 0;
    if (millis() - lastLog1 > 300) {
      Serial.printf("MODO 1 | ADC: %d | PWM: %d\n", valorADC, valorPWM);
      lastLog1 = millis();
    }
  }

  else if (modoActual == 2) {
    ledcWrite(led1Pin, 0);

    int valorPot2 = analogRead(pot2Pin);
    int freqParpadeo = map(valorPot2, 0, 4095, 20, 1);

    static int freqAnterior = 0;
    if (abs(freqParpadeo - freqAnterior) >= 1) {
      configurarTimer(freqParpadeo);
      freqAnterior = freqParpadeo;
    }

    if (timerFlag) {
      timerFlag = false;
      estadoLed2 = !estadoLed2;
      digitalWrite(led2Pin, estadoLed2);
      
      Serial.printf("MODO 2 | Freq: %d Hz | LED: %s\n", freqParpadeo, estadoLed2 ? "ON" : "OFF");
    }
  }
}
