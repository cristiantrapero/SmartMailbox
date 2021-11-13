#include <Arduino.h>
#include <WiFi.h>
#include "Keypad.h"
#include "NimBLEDevice.h"

// WIFI credentials
const char *WIFI_SSID = "test";
const char *WIFI_PASSWORD = "test";
const String HOSTNAME = "SmartMailbox";

// Bluetooth serial
#define SERVICE_UUID "2af412d8-3e7e-11ec-9bbc-0242ac130002"
#define CHARACTERISTIC_UUID "b50b1050-196d-42df-bfdc-674c31ec2699"
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

// Keypad size
const byte ROWS = 4;
const byte COLS = 4;

// Keypad keys distribution
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Keypad pinout and variables
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
const String password = "1234"; //TODO: Get from eeprom
String inputPassword;

// Constants declarations
const int CALL_BUTTON = 4;
const int DOOR_RELAY = 15;
const int ECHO = 19;
const int TRIGGER = 18;

// Auxiliar variables
int pressedButton = 0;
long usDistance;
long usTime;

// Functions prototype declaration
void connectWiFi();
void enableBluetooth();
void openMailbox();
void sendMailboxOpeningNotification();
void evaluateSendLetterNotification(int usDistance);
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

void setup()
{
  Serial.begin(9600);

  // WiFi configuration
  // WiFi event on disconnect, retry the conexion
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  connectWiFi();

  // Bluetooth configuration
  enableBluetooth();

  // Ultrasonic sensor
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIGGER, LOW);

  // Call button
  pinMode(CALL_BUTTON, INPUT);

  // Relay
  pinMode(DOOR_RELAY, OUTPUT);
}

void loop()
{

  // Keypad input
  char pressedKey = keypad.getKey();

  if (pressedKey)
  {
    Serial.println(pressedKey);

    if (pressedKey == '*')
    {
      inputPassword = "";
    }
    else if (pressedKey == '#')
    {
      if (password == inputPassword)
      {
        Serial.println("Password is correct. Opening mailbox.");
        openMailbox();
      }
      else
      {
        Serial.println("Password is incorrect, try again.");
      }

      inputPassword = ""; // clear input password
    }
    else
    {
      inputPassword += pressedKey;
    }
  }

  // Call open mailbox button
  pressedButton = digitalRead(CALL_BUTTON);

  if (pressedButton == HIGH)
  {
    sendMailboxOpeningNotification();
  }

  // Ultrasonic calculations
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  usTime = pulseIn(ECHO, HIGH);
  usDistance = usTime / 59;
  evaluateSendLetterNotification(usDistance);
}

void sendMailboxOpeningNotification(){
  // TODO: Send mailbox opening notification
  Serial.println("Send mailbox opening notification");
}

void openMailbox()
{
  digitalWrite(DOOR_RELAY, HIGH);
  delay(5000);
  digitalWrite(DOOR_RELAY, LOW);
  // TODO: send opened notification
}

void evaluateSendLetterNotification(int usDistance)
{
  // TODO: Add delay for false positive
  if (usDistance < 7)
  {
    Serial.println("Send recieved letter notification");
  }
}

void connectWiFi()
{
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(HOSTNAME.c_str());
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

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("Device connected");
  };

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Device disconnected");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      Serial.println("*********");
      Serial.print("Received value: ");
      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i]);

      Serial.println();
      Serial.println("*********");
    }
  }
};

void enableBluetooth()
{
  BLEDevice::init("SmartMailbox");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("\nThe " + HOSTNAME + " bluetooth started\n");
}
