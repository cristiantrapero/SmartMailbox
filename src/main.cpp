#include <Arduino.h>
#include <WiFi.h>
#include <U8g2lib.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "Keypad.h"
#include "NimBLEDevice.h"
#include "time.h"

// WIFI credentials
const char *WIFI_SSID = "test";
const char *WIFI_PASSWORD = "test";
const String HOSTNAME = "SmartMailbox";

// Bluetooth serial
#define SERVICE_UUID "2af412d8-3e7e-11ec-9bbc-0242ac130002"
#define CHARACTERISTIC_UUID "b50b1050-196d-42df-bfdc-674c31ec2699"
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

// NTP configuration
const char* NTPSERVER = "pool.ntp.org";
const long  GMTOFFSET = 1;
const int   DAYLIGHTOFFSET = 3600;

// MQTT configuration
const char* MQTTSERVER = "mqtt.cristiantrapero.es";
const int MQTTPORT = 1883;
const char* MQTTUSER = "test";
const char* MQTTPASSWORD = "test";
WiFiClient espClient;
PubSubClient client(espClient);

const char* PINTOPIC = "/smartmailbox/pin";
const char* RELAYTOPIC = "/smartmailbox/relay";
const char* LETTERTOPIC = "/smartmailbox/letter";
const char* OPENTOPIC = "/smartmailbox/open";
const char* OPENEDTOPIC = "/smartmailbox/opened";

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
String mailboxPIN = "1234"; //TODO: Get from eeprom
String inputPIN;

// Constants declarations
const int CALL_BUTTON = 4;
const int DOOR_RELAY = 15;
const int ECHO = 19;
const int TRIGGER = 18;

// Auxiliar variables
int pressedButton = 0;
long usDistance;
long usTime;
unsigned long lastLetterCall;
unsigned long lastOpenCall;

// EEPROM to save PINCODE. MAX 8 digits
int pinAddr = 0;
#define EEPROM_SIZE 8

// Functions prototype declaration
void connectToWiFi();
void enableBluetooth();
void changePasswordPin(String pin);
void openMailbox();
void reconnectMQTT();
void sendTimeMessageToTopic(const char* topic);
void sendOpenMailboxRequest();
void sendLetterNotification(int usDistance);
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void mqttCallback(char* topic, byte* message, unsigned int length);
String read_word(int addr);
void write_word(int addr, String word);

void setup()
{
  Serial.begin(9600);

  // EEPROM for pincode
  EEPROM.begin(EEPROM_SIZE);
  mailboxPIN = read_word(pinAddr);

  // WiFi configuration
  // WiFi event on disconnect, retry the conexion
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  connectToWiFi();

  // NTP
  configTime(GMTOFFSET, DAYLIGHTOFFSET, NTPSERVER);

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

  // MQTT
  client.setServer(MQTTSERVER, MQTTPORT);
  client.setCallback(mqttCallback);
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String command;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    command += (char)message[i];
  }
  Serial.println();
  if (String(topic) == RELAYTOPIC) {
    if(command == "on"){
      openMailbox();
    }
  } else if (String(topic) == PINTOPIC){
      changePasswordPin(command);
  }
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
      inputPIN = "";
    }
    else if (pressedKey == '#')
    {
      if (mailboxPIN == inputPIN)
      {
        Serial.println("Password is correct. Opening mailbox.");
        openMailbox();
      }
      else
      {
        Serial.println("Password is incorrect, try again.");
      }

      inputPIN = "";
    }
    else
    {
      inputPIN += pressedKey;
    }
  }

  // Call open mailbox button
  pressedButton = digitalRead(CALL_BUTTON);

  if (pressedButton == HIGH)
  {
    sendOpenMailboxRequest();
  }

  // Ultrasonic calculations
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  usTime = pulseIn(ECHO, HIGH);
  usDistance = usTime / 59;
  sendLetterNotification(usDistance);

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("SmartMailbox", MQTTUSER, MQTTPASSWORD)) {
      Serial.println("\nMQTT connected");
      client.subscribe(PINTOPIC);
      client.subscribe(RELAYTOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendOpenMailboxRequest(){
  if (millis() - lastOpenCall >= 5*1000UL){
    lastOpenCall = millis();
    Serial.println("Send mailbox open request");
    sendTimeMessageToTopic(OPENTOPIC);
  }
}

void changePasswordPin(String pin){
  mailboxPIN = pin;
  write_word(pinAddr, pin);
}

String read_word(int addr)
{
  String word;
  char readChar;
  int i = addr;

  while (readChar != '\0')
  {
    readChar = char(EEPROM.read(i));
    delay(10);
    i++;

    if (readChar != '\0')
    {
      word += readChar;
    }
  }

  return word;
}

void write_word(int addr, String word)
{
  delay(10);
  int str_len = word.length() + 1;

  for (int i = addr; i < str_len + addr; ++i)
  {
    EEPROM.write(i, word.charAt(i - addr));
  }

  EEPROM.write(str_len + addr, '\0');
  EEPROM.commit();
}

void openMailbox()
{
  Serial.println("Open smartmailbox");
  Serial.println("Send mailbox opened notification");
  sendTimeMessageToTopic(OPENEDTOPIC);
  digitalWrite(DOOR_RELAY, HIGH);
  delay(5000);
  digitalWrite(DOOR_RELAY, LOW);
}

void sendLetterNotification(int usDistance)
{
    if (usDistance < 7)
    {
      if (millis() - lastLetterCall >= 10*1000UL){
        lastLetterCall = millis();
        Serial.println("Send mailbox letter notification");
        sendTimeMessageToTopic(LETTERTOPIC);
    }
  }
}

void sendTimeMessageToTopic(const char* topic) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timestamp[50];
  strftime(timestamp, sizeof(timestamp), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  client.publish(topic, timestamp);
}

void connectToWiFi()
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
  connectToWiFi();
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
  Serial.println("\nThe " + HOSTNAME + " bluetooth started");
}
