#include <Arduino.h>
#include <WiFi.h>
#include "Keypad.h"
#include "NimBLEDevice.h"

// WIFI credentials
const char *WIFI_SSID = "test";
const char *WIFI_PASSWORD = "test";
const String hostname = "SmartMailbox";

// Bluetooth serial
#define SERVICE_UUID "2af412d8-3e7e-11ec-9bbc-0242ac130002"
#define CHARACTERISTIC_UUID "b50b1050-196d-42df-bfdc-674c31ec2699"

// Keypad size
const byte ROWS = 4;
const byte COLS = 4;

// Keypad keys distribution
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Keypad pinout
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Functions prototype declaration
void connectWiFi();
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void enableBluetooth();

void setup()
{
  Serial.begin(9600);

  // WiFi configuration
  // WiFi event on disconnect, retry the conexion
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  connectWiFi();

  // Bluetooth configuration
  enableBluetooth();
}

void loop()
{

  char pressedKey = keypad.getKey();

  if (pressedKey)
  {
    Serial.println(pressedKey);
  }
}

void connectWiFi()
{
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to the router " + String(WIFI_SSID) + " with password " + String(WIFI_PASSWORD) + "\n");
  Serial.print("Trying");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }

  Serial.print("\nSmartMailbox connected to " + String(WIFI_SSID) + " with IP: ");
  Serial.print(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  connectWiFi();
}

void enableBluetooth()
{
  BLEDevice::init("SmartMailbox");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ |
          NIMBLE_PROPERTY::WRITE);

  pCharacteristic->setValue("SmartMailbox");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("\nThe " + hostname + " bluetooth started\n");
}
