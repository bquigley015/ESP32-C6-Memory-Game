//memory game for Seeed XIAO ESP32 C6
//LED pins: D0 red, D1 yellow, D2 green, D3 blue (through resistors to GND)
//Button pins: D4 red, D5 yellow, D7 green, D8 blue 

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

const char* WIFI_SSID = ""; //set to default when pushing
const char* WIFI_PASS = ""; //set to default when pushing


const int LED_PINS[4] = {D0, D1, D2, D3};
const int BTN_PINS[4] = {D4, D5, D7, D8};

const int MAX_LEVEL = 25;
int sequence[MAX_LEVEL];
int level = 0;

const int TOP = 10;
int scores[TOP];        
int gameNums[TOP];      
int gamesPlayed = 0;
Preferences prefs;
WebServer server(80);

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }
  prefs.begin("memgame", false);
  for (int i = 0; i < TOP; i++) {
    scores[i]   = prefs.getInt(("s" + String(i)).c_str(), 0);
    gameNums[i] = prefs.getInt(("g" + String(i)).c_str(), 0);
  }
  gamesPlayed = prefs.getInt("games", 0);

#if defined(WIFI_ENABLE) && defined(WIFI_ANT_CONFIG)
  pinMode(WIFI_ENABLE, OUTPUT);     
  digitalWrite(WIFI_ENABLE, LOW);
  delay(100);
  pinMode(WIFI_ANT_CONFIG, OUTPUT); digitalWrite(WIFI_ANT_CONFIG, LOW);
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wi-Fi");
  for (int t = 0; t < 30 && WiFi.status() != WL_CONNECTED; t++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  server.on("/", showLeaderboard);
  server.begin();

  xTaskCreate([](void*) { while (true) { server.handleClient(); delay(2); } },
              "web", 4096, NULL, 1, NULL);

  printAddress();
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
      recordScore(level - 1);
      gameOver();
      break;
    }
    if (level >= MAX_LEVEL) { 
      recordScore(MAX_LEVEL);
      winAnimation();
      break;
    }
    delay(600);
  }
}

void recordScore(int score) {
  gamesPlayed++;
  prefs.putInt("games", gamesPlayed);
  Serial.printf("Game %d: scored %d\n", gamesPlayed, score);
  if (score <= scores[TOP - 1]) {
    return;     
  }

  int pos = TOP - 1;
  while (pos > 0 && scores[pos - 1] < score) {
    scores[pos] = scores[pos - 1];
    gameNums[pos] = gameNums[pos - 1];
    pos--;
  }
  scores[pos] = score;
  gameNums[pos] = gamesPlayed;

  for (int i = 0; i < TOP; i++) {
    prefs.putInt(("s" + String(i)).c_str(), scores[i]);
    prefs.putInt(("g" + String(i)).c_str(), gameNums[i]);
  }

  Serial.printf("New #%d score!\n", pos + 1);
  printAddress();
}

void showLeaderboard() {
  String html = "<html><head><meta name='viewport' content='width=device-width'>"
                "<meta http-equiv='refresh' content='5'><title>Leaderboard</title></head>"
                "<body style='font-family:sans-serif'><h1>Memory Game Leaderboard</h1>"
                "<table border='1' cellpadding='6'><tr><th>Rank</th><th>Score</th><th>Game #</th></tr>";
  for (int i = 0; i < TOP && scores[i] > 0; i++) {
    html += "<tr><td>" + String(i + 1) + "</td><td>" + String(scores[i]) +
            "</td><td>" + String(gameNums[i]) + "</td></tr>";
  }

  html += "</table><p>Games played: " + String(gamesPlayed) + "</p></body></html>";
  server.send(200, "text/html", html);
}

void printAddress() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Leaderboard: http://%s/\n", WiFi.localIP().toString().c_str());

  } else {
    Serial.println("Wi-Fi not connected - leaderboard page unavailable");
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
    delay(1);
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
