//memory game for Seeed XIAO ESP32 C6
//LED pins: D0 red, D1 yellow, D2 green, D3 blue (through resistors to GND)
//Button pins: D4 red, D5 yellow, D7 green, D8 blue 

const int LED_PINS[4] = {D0, D1, D2, D3};
const int BTN_PINS[4] = {D4, D5, D7, D8};

const int MAX_LEVEL = 25;
int sequence[MAX_LEVEL];
int level = 0;


void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }
  randomSeed(esp_random());
  startupAnimation();
}

void loop() {
  waitForAnyPress();          
  delay(500);
  level = 0;

  while (true) {
    sequence[level] = random(4);
    level++;
    playSequence();
    if (!readPlayerInput()) {
      gameOver();
      break;
    }
    if (level >= MAX_LEVEL) {  
      winAnimation();
      break;
    }
    delay(600);
  }
}



void flash(int color, int onMs) {
  digitalWrite(LED_PINS[color], HIGH);
  delay(onMs);
  digitalWrite(LED_PINS[color], LOW);
}

void playSequence() {
  
  int onMs = max(150, 500 - level * 15);
  for (int i = 0; i < level; i++) {
    flash(sequence[i], onMs);
    delay(onMs / 2);
  }
}


int readButton(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    for (int i = 0; i < 4; i++) {
      if (digitalRead(BTN_PINS[i]) == LOW) {
        delay(20);                                   
        if (digitalRead(BTN_PINS[i]) == LOW) {
          digitalWrite(LED_PINS[i], HIGH);          
          while (digitalRead(BTN_PINS[i]) == LOW) {
            delay(5);
          }

          digitalWrite(LED_PINS[i], LOW);
          delay(20);
          return i;
        }
      }
    }
  }
  return -1;
}

bool readPlayerInput() {
  for (int i = 0; i < level; i++) {
    int pressed = readButton(3000);                  
    if (pressed != sequence[i]) return false;
  }
  return true;
}

void waitForAnyPress() {
  
  int i = 0;
  while (true) {
    digitalWrite(LED_PINS[i], HIGH);
    if (readButton(250) >= 0) { 
      allOff(); 
      return; 
    }

    digitalWrite(LED_PINS[i], LOW);
    i = (i + 1) % 4;
  }
}

void allOff() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

void allOn() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }
}

void startupAnimation() {
  for (int r = 0; r < 2; r++) {
    for (int i = 0; i < 4; i++) {
      flash(i, 100);
    }
  }
}

void gameOver() {
  Serial.printf("Game over! You reached level %d\n", level - 1);
  for (int r = 0; r < 3; r++) {
    allOn();  delay(200);
    allOff(); delay(200);
  }
  
  delay(500);
  for (int s = 0; s < level - 1; s++) { 
    flash(0, 150); 
    delay(150); 
  }
  delay(1000);
}

void winAnimation() {
  for (int r = 0; r < 5; r++) {
    startupAnimation();
  }
}
