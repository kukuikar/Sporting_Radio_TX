#include <esp_now.h>
#include <WiFi.h>

#define NUM_BUTTONS 12

const char buttonNames[NUM_BUTTONS] = {'A','B','C','D','E','F','G','H','Z','M','K','O'};
const uint8_t buttonPins[NUM_BUTTONS] = {34, 35, 32, 33, 25, 26, 27, 14, 12, 13, 4, 15};
const uint8_t ledPin = 2;

struct ButtonInfo {
  uint8_t pin;
  bool state;
  char name;
};

struct ButtonMessage {
  ButtonInfo buttons[NUM_BUTTONS];
};

ButtonMessage msg;

bool lastStates[NUM_BUTTONS];
unsigned long lastDebounceTime[NUM_BUTTONS];
const unsigned long debounceDelay = 30;

uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x12, 0x34, 0x56};

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (true);
  }

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT);
    lastStates[i] = false;
    lastDebounceTime[i] = 0;

    msg.buttons[i].pin = buttonPins[i];
    msg.buttons[i].state = false;
    msg.buttons[i].name = buttonNames[i];
  }
}

void loop() {
  static unsigned long lastSendTime = 0;
  bool changed = false;
  bool anyPressed = false;

  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool reading = digitalRead(buttonPins[i]);

    if (reading != lastStates[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != msg.buttons[i].state) {
        msg.buttons[i].state = reading;
        changed = true;
      }
    }

    lastStates[i] = reading;

    if (msg.buttons[i].state) {
      anyPressed = true;
    }
  }

  digitalWrite(ledPin, anyPressed ? HIGH : LOW);

  if (changed && (millis() - lastSendTime > 50)) {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&msg, sizeof(msg));
    if (result == ESP_OK) {
      Serial.println("Message sent");
    } else {
      Serial.println("Send error");
    }
    lastSendTime = millis();
  }
}